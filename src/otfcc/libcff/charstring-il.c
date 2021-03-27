#include "charstring-il.h"
#include "table/glyf.h"

// Glyph building
static void ensureThereIsSpace(cff_CharstringIL *il) {
	if (il->free) return;
	il->free = 0x100;
	RESIZE(il->instr, il->length + il->free);
}

void il_push_operand(cff_CharstringIL *il, double x) {
	ensureThereIsSpace(il);
	il->instr[il->length].type = IL_ITEM_OPERAND;
	il->instr[il->length].d = x;
	il->instr[il->length].arity = 0;
	il->length++;
	il->free--;
}
void il_push_VQ(cff_CharstringIL *il, VQ x) {
	il_push_operand(il, iVQ.getStill(x));
}
void il_push_special(cff_CharstringIL *il, int32_t s) {
	ensureThereIsSpace(il);
	il->instr[il->length].type = IL_ITEM_SPECIAL;
	il->instr[il->length].i = s;
	il->instr[il->length].arity = 0;
	il->length++;
	il->free--;
}
void il_push_op(cff_CharstringIL *il, int32_t op) {
	ensureThereIsSpace(il);
	il->instr[il->length].type = IL_ITEM_OPERATOR;
	il->instr[il->length].i = op;
	il->instr[il->length].arity = cff_getStandardArity(op);
	il->length++;
	il->free--;
}
static void il_moveto(cff_CharstringIL *il, VQ dx, VQ dy) {
	il_push_VQ(il, dx);
	il_push_VQ(il, dy);
	il_push_op(il, op_rmoveto);
}
static void il_lineto(cff_CharstringIL *il, VQ dx, VQ dy) {
	il_push_VQ(il, dx);
	il_push_VQ(il, dy);
	il_push_op(il, op_rlineto);
}
static void il_curveto(cff_CharstringIL *il, VQ dx1, VQ dy1, VQ dx2, VQ dy2, VQ dx3, VQ dy3) {
	il_push_VQ(il, dx1);
	il_push_VQ(il, dy1);
	il_push_VQ(il, dx2);
	il_push_VQ(il, dy2);
	il_push_VQ(il, dx3);
	il_push_VQ(il, dy3);
	il_push_op(il, op_rrcurveto);
}

static void _il_push_maskgroup(cff_CharstringIL *il,     // il seq
                               glyf_MaskList *masks,     // masks array
                               uint16_t contours,        // contous drawn
                               uint16_t points,          // points drawn
                               uint16_t nh, uint16_t nv, // quantity of stems
                               uint16_t *jm,             // index of cur mask
                               int32_t op) {             // mask operator
	shapeid_t n = masks->length;
	while (*jm < n && (masks->items[*jm].contoursBefore < contours ||
	                   (masks->items[*jm].contoursBefore == contours &&
	                    masks->items[*jm].pointsBefore <= points))) {
		il_push_op(il, op);
		uint8_t maskByte = 0;
		uint8_t bits = 0;
		for (uint16_t j = 0; j < nh; j++) {
			maskByte = maskByte << 1 | (masks->items[*jm].maskH[j] & 1);
			bits += 1;
			if (bits == 8) {
				il_push_special(il, maskByte);
				bits = 0;
			}
		}
		for (uint16_t j = 0; j < nv; j++) {
			maskByte = maskByte << 1 | (masks->items[*jm].maskV[j] & 1);
			bits += 1;
			if (bits == 8) {
				il_push_special(il, maskByte);
				bits = 0;
			}
		}
		if (bits) {
			maskByte = maskByte << (8 - bits);
			il_push_special(il, maskByte);
		}
		*jm += 1;
	}
}
static void il_push_masks(cff_CharstringIL *il, glyf_Glyph *g, // meta
                          uint16_t contours,                   // contours sofar
                          uint16_t points,                     // points sofar
                          uint16_t *jh,                        // index of pushed cmasks
                          uint16_t *jm                         // index of pushed hmasks
) {
	if (!g->stemH.length && !g->stemV.length) return;
	_il_push_maskgroup(il, &g->contourMasks, contours, points, //
	                   g->stemH.length, g->stemV.length, jh, op_cntrmask);
	_il_push_maskgroup(il, &g->hintMasks, contours, points, //
	                   g->stemH.length, g->stemV.length, jm, op_hintmask);
}

static void _il_push_stemgroup(cff_CharstringIL *il,    // il seq
                               glyf_StemDefList *stems, // stem array
                               bool hasmask, bool haswidth, int32_t ophm, int32_t oph) {
	if (!stems || !stems->length) return;
	pos_t ref = 0;
	uint16_t nn = haswidth ? 1 : 0;
	for (uint16_t j = 0; j < stems->length; j++) {
		il_push_operand(il, stems->items[j].position - ref);
		il_push_operand(il, stems->items[j].width);
		ref = stems->items[j].position + stems->items[j].width;
		nn++;
		if (nn >= type2_argument_stack) {
			if (hasmask) {
				il_push_op(il, op_hstemhm);
			} else {
				il_push_op(il, op_hstem);
			}
			il->instr[il->length - 1].arity = nn;
			nn = 0;
		}
	}
	if (hasmask) {
		il_push_op(il, ophm);
	} else {
		il_push_op(il, oph);
	}
	il->instr[il->length - 1].arity = nn;
}
static void il_push_stems(cff_CharstringIL *il, glyf_Glyph *g, bool hasmask, bool haswidth) {
	_il_push_stemgroup(il, &g->stemH, hasmask, haswidth, op_hstemhm, op_hstem);
	_il_push_stemgroup(il, &g->stemV, hasmask, haswidth, op_vstemhm, op_vstem);
}
cff_CharstringIL *cff_compileGlyphToIL(glyf_Glyph *g, uint16_t defaultWidth,
                                       uint16_t nominalWidth) {
	cff_CharstringIL *il;
	NEW(il);
	// Convert absolute positions to deltas
	glyf_Contour *tempContours = NULL;
	{
		VQ x = iVQ.neutral();
		VQ y = iVQ.neutral();
		NEW(tempContours, g->contours.length);
		for (uint16_t c = 0; c < g->contours.length; c++) {
			glyf_Contour *contour = &(g->contours.items[c]);
			glyf_Contour *newcontour = &(tempContours[c]);
			glyf_iContour.init(newcontour);
			for (shapeid_t j = 0; j < contour->length; j++) {
				glyf_iContour.push(newcontour, glyf_iPoint.dup(contour->items[j]));
			}

			if (newcontour->length > 2 && !newcontour->items[newcontour->length - 1].onCurve) {
				// Duplicate first point for proper CurveTo generation
				glyf_iContour.push(newcontour, glyf_iPoint.dup(newcontour->items[0]));
			}

			for (shapeid_t j = 0; j < newcontour->length; j++) {
				VQ dx = iVQ.minus(newcontour->items[j].x, x);
				VQ dy = iVQ.minus(newcontour->items[j].y, y);
				iVQ.copyReplace(&x, newcontour->items[j].x);
				iVQ.copyReplace(&y, newcontour->items[j].y);
				iVQ.replace(&newcontour->items[j].x, dx);
				iVQ.replace(&newcontour->items[j].y, dy);
			}
		}
		iVQ.dispose(&x);
		iVQ.dispose(&y);
	}

	bool hasmask =
	    g->hintMasks.length || g->contourMasks.length; // we have hint masks or contour masks
	const pos_t glyphADWConst = iVQ.getStill(g->advanceWidth);
	bool haswidth = glyphADWConst != defaultWidth; // we have width operand here
	// Write IL
	if (haswidth) { il_push_operand(il, (int)(glyphADWConst) - (int)(nominalWidth)); }
	il_push_stems(il, g, hasmask, haswidth);
	// Write contour
	shapeid_t contoursSofar = 0;
	shapeid_t pointsSofar = 0;
	shapeid_t jh = 0;
	shapeid_t jm = 0;
	if (hasmask) il_push_masks(il, g, contoursSofar, pointsSofar, &jh, &jm);
	for (shapeid_t c = 0; c < g->contours.length; c++) {
		glyf_Contour *contour = &(tempContours[c]);
		shapeid_t n = contour->length;
		if (n == 0) continue;
		il_moveto(il, contour->items[0].x, contour->items[0].y);
		pointsSofar++;
		if (hasmask) il_push_masks(il, g, contoursSofar, pointsSofar, &jh, &jm);
		// TODO: Generate BLENDs
		for (shapeid_t j = 1; j < n; j++) {
			if (contour->items[j].onCurve) { // A line-to
				il_lineto(il, contour->items[j].x, contour->items[j].y);
				pointsSofar += 1;
			} else if (j < n - 2                         // have enough points
			           && !contour->items[j + 1].onCurve // next is offcurve
			           && contour->items[j + 2].onCurve  // and next is oncurve
			) {                                          // means this is an bezier curve strand
				il_curveto(il, contour->items[j].x,
				           contour->items[j].y, // dz1
				           contour->items[j + 1].x,
				           contour->items[j + 1].y, // dz2
				           contour->items[j + 2].x,
				           contour->items[j + 2].y); // dz3
				pointsSofar += 3;
				j += 2;
			} else { // invalid offcurve, treat as oncurve
				il_lineto(il, contour->items[j].x, contour->items[j].y);
				pointsSofar++;
			}
			if (hasmask) il_push_masks(il, g, contoursSofar, pointsSofar, &jh, &jm);
		}
		contoursSofar += 1;
		pointsSofar = 0;
	}
	il_push_op(il, op_endchar);
	// delete temp contour array
	for (shapeid_t c = 0; c < g->contours.length; c++) {
		glyf_iContour.dispose(&tempContours[c]);
	}
	FREE(tempContours);
	return il;
}

// Pattern-based peephole optimization
static bool il_matchtype(cff_CharstringIL *il, uint32_t j, uint32_t k, cff_InstructionType t) {
	if (k >= il->length) return false;
	for (uint32_t m = j; m < k; m++) {
		if (il->instr[m].type != t) return false;
	}
	return true;
}
static bool il_matchop(cff_CharstringIL *il, uint32_t j, int32_t op) {
	if (il->instr[j].type != IL_ITEM_OPERATOR) return false;
	if (il->instr[j].i != op) return false;
	return true;
}
static uint8_t zroll(cff_CharstringIL *il, uint32_t j, int32_t op, int32_t op2, ...) {
	uint8_t arity = cff_getStandardArity(op);
	if (arity > 16 || j + arity >= il->length) return 0;
	if ((j == 0 || // We are at the beginning of charstring
	     !il_matchtype(il, j - 1, j,
	                   IL_ITEM_PHANTOM_OPERATOR)) // .. or we are right after a solid operator
	    && il_matchop(il, j + arity, op)          // The next operator is <op>
	    && il_matchtype(il, j, j + arity, IL_ITEM_OPERAND) // And we have correct number of operands
	) {
		va_list ap;
		uint8_t check = true;
		uint8_t resultArity = arity;
		bool mask[16];
		va_start(ap, op2);
		for (uint32_t m = 0; m < arity; m++) {
			int checkzero = va_arg(ap, int);
			mask[m] = checkzero;
			if (checkzero) {
				resultArity -= 1;
				check = check && il->instr[j + m].d == 0;
			}
		}
		va_end(ap);
		if (check) {
			for (uint32_t m = 0; m < arity; m++) {
				if (mask[m]) { il->instr[j + m].type = IL_ITEM_PHANTOM_OPERAND; }
			}
			il->instr[j + arity].i = op2;
			il->instr[j + arity].arity = resultArity;
			return arity;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}
static uint8_t opop_roll(cff_CharstringIL *il, uint32_t j, int32_t op1, int32_t arity, int32_t op2,
                         int32_t resultop) {
	if (j + 1 + arity >= il->length) return 0;
	cff_CharstringInstruction *current = &(il->instr[j]);
	cff_CharstringInstruction *nextop = &(il->instr[j + 1 + arity]);
	if (il_matchop(il, j, op1)                                     // match this operator
	    && il_matchtype(il, j + 1, j + 1 + arity, IL_ITEM_OPERAND) // match operands
	    && il_matchop(il, j + 1 + arity, op2)                      // match next operator
	    && current->arity + nextop->arity <= type2_argument_stack  // stack is not full
	) {
		current->type = IL_ITEM_PHANTOM_OPERATOR;
		nextop->i = resultop;
		nextop->arity += current->arity;
		return arity + 1;
	} else {
		return 0;
	}
}
static uint8_t hvlineto_roll(cff_CharstringIL *il, uint32_t j) {
	if (j + 3 >= il->length) return 0;
	cff_CharstringInstruction *current = &(il->instr[j]);
	// We will check whether operand <checkdelta> is zero
	//          ODD     EVEN -- current arity
	// hlineto   X       Y
	// vlineto   Y       X
	uint32_t checkdelta = ((bool)(current->arity & 1) ^ (bool)(current->i == op_vlineto) ? 1 : 2);
	if ((il_matchop(il, j, op_hlineto) || il_matchop(il, j, op_vlineto)) // a hlineto/vlineto
	    && il_matchop(il, j + 3, op_rlineto)                             // followed by a lineto
	    && il_matchtype(il, j + 1, j + 3, IL_ITEM_OPERAND)               // have enough operands
	    && il->instr[j + checkdelta].d == 0                              // and it is a h/v
	    && current->arity + 1 <= type2_argument_stack // we have enough stack space
	) {
		il->instr[j + checkdelta].type = IL_ITEM_PHANTOM_OPERAND;
		il->instr[j].type = IL_ITEM_PHANTOM_OPERATOR;
		il->instr[j + 3].i = current->i;
		il->instr[j + 3].arity = current->arity + 1;
		return 3;
	} else {
		return 0;
	}
}
static uint8_t hvvhcurve_roll(cff_CharstringIL *il, uint32_t j) {
	if (!il_matchop(il, j, op_hvcurveto) && !il_matchop(il, j, op_vhcurveto)) return 0;
	cff_CharstringInstruction *current = &(il->instr[j]);
	// Exit in case of array not long enough or we have already ended
	if (j + 7 >= il->length || current->arity & 1) return 0;
	bool hvcase = (bool)((current->arity >> 2) & 1) ^ (bool)(current->i == op_hvcurveto);
	// We will check whether operand <checkdelta> is zero
	//            ODD     EVEN -- current arity divided by 4
	// hvcurveto   X       Y
	// vhcurveto   Y       X
	uint32_t checkdelta1 = hvcase ? 2 : 1;
	uint32_t checkdelta2 = hvcase ? 5 : 6;
	if (il_matchop(il, j + 7, op_rrcurveto)                // followed by a curveto
	    && il_matchtype(il, j + 1, j + 7, IL_ITEM_OPERAND) // have enough operands
	    && il->instr[j + checkdelta1].d == 0               // and it is a h/v
	) {
		if (il->instr[j + checkdelta2].d == 0 && current->arity + 4 <= type2_argument_stack) {
			// The Standard case
			il->instr[j + checkdelta1].type = IL_ITEM_PHANTOM_OPERAND;
			il->instr[j + checkdelta2].type = IL_ITEM_PHANTOM_OPERAND;
			il->instr[j].type = IL_ITEM_PHANTOM_OPERATOR;
			il->instr[j + 7].i = current->i;
			il->instr[j + 7].arity = current->arity + 4;
			return 7;
		} else if (current->arity + 5 <= type2_argument_stack) {
			// The trailing case
			il->instr[j + checkdelta1].type = IL_ITEM_PHANTOM_OPERAND;
			il->instr[j].type = IL_ITEM_PHANTOM_OPERATOR;
			il->instr[j + 7].i = current->i;
			il->instr[j + 7].arity = current->arity + 5;
			if (hvcase) {
				// Swap the last two operands because hvcurveto's trailing operand is in y-x order
				double t = il->instr[j + 5].d;
				il->instr[j + 5].d = il->instr[j + 6].d;
				il->instr[j + 6].d = t;
			}
			return 7;
		} else {
			return 0;
		}
	} else {
		return 0;
	}
}
static uint8_t hhvvcurve_roll(cff_CharstringIL *il, uint32_t j) {
	if (!il_matchop(il, j, op_hhcurveto) && !il_matchop(il, j, op_vvcurveto)) return 0;
	cff_CharstringInstruction *current = &(il->instr[j]);
	// Exit in case of array not long enough or we have already ended
	if (j + 7 >= il->length) return 0;
	bool hh = current->i == op_hhcurveto;
	uint32_t checkdelta1 = hh ? 2 : 1;
	uint32_t checkdelta2 = hh ? 6 : 5;
	if (il_matchop(il, j + 7, op_rrcurveto)                // followed by a curveto
	    && il_matchtype(il, j + 1, j + 7, IL_ITEM_OPERAND) // have enough operands
	    && il->instr[j + checkdelta1].d == 0               // and it is a h/v
	    && il->instr[j + checkdelta2].d == 0               // and it is a h/v
	    && current->arity + 4 <= type2_argument_stack) {
		il->instr[j + checkdelta1].type = IL_ITEM_PHANTOM_OPERAND;
		il->instr[j + checkdelta2].type = IL_ITEM_PHANTOM_OPERAND;
		il->instr[j].type = IL_ITEM_PHANTOM_OPERATOR;
		il->instr[j + 7].i = current->i;
		il->instr[j + 7].arity = current->arity + 4;
		return 7;
	} else {
		return 0;
	}
}
static uint32_t nextstop(cff_CharstringIL *il, uint32_t j) {
	uint32_t delta = 0;
	for (; j + delta < il->length && il->instr[j + delta].type == IL_ITEM_OPERAND; delta++)
		;
	return delta;
}
#define ROLL_FALL(x)                                                                               \
	if ((r = (x))) return r;
static uint8_t decideAdvance(cff_CharstringIL *il, uint32_t j, uint8_t optimizeLevel) {
	uint8_t r = 0;
	ROLL_FALL(zroll(il, j, op_rlineto, op_hlineto, 0, 1));                 // rlineto -> hlineto
	ROLL_FALL(zroll(il, j, op_rlineto, op_vlineto, 1, 0));                 // rlineto -> vlineto
	ROLL_FALL(zroll(il, j, op_rmoveto, op_hmoveto, 0, 1));                 // rmoveto -> hmoveto
	ROLL_FALL(zroll(il, j, op_rmoveto, op_vmoveto, 1, 0));                 // rmoveto -> vmoveto
	ROLL_FALL(zroll(il, j, op_rrcurveto, op_hvcurveto, 0, 1, 0, 0, 1, 0)); // rrcurveto->hvcurveto
	ROLL_FALL(zroll(il, j, op_rrcurveto, op_vhcurveto, 1, 0, 0, 0, 0, 1)); // rrcurveto->vhcurveto
	ROLL_FALL(zroll(il, j, op_rrcurveto, op_hhcurveto, 0, 1, 0, 0, 0, 1)); // rrcurveto->hhcurveto
	ROLL_FALL(zroll(il, j, op_rrcurveto, op_vvcurveto, 1, 0, 0, 0, 1, 0)); // rrcurveto->vvcurveto
	ROLL_FALL(opop_roll(il, j, op_rrcurveto, 6, op_rrcurveto, op_rrcurveto)); // rrcurveto roll
	ROLL_FALL(opop_roll(il, j, op_rrcurveto, 2, op_rlineto, op_rcurveline));  // rcurveline roll
	ROLL_FALL(opop_roll(il, j, op_rlineto, 6, op_rrcurveto, op_rlinecurve));  // rlinecurve roll
	ROLL_FALL(opop_roll(il, j, op_rlineto, 2, op_rlineto, op_rlineto));       // rlineto roll
	ROLL_FALL(opop_roll(il, j, op_hstemhm, 0, op_hintmask, op_hintmask));     // hintmask roll
	ROLL_FALL(opop_roll(il, j, op_vstemhm, 0, op_hintmask, op_hintmask));     // hintmask roll
	ROLL_FALL(opop_roll(il, j, op_hstemhm, 0, op_cntrmask, op_cntrmask));     // cntrmask roll
	ROLL_FALL(opop_roll(il, j, op_vstemhm, 0, op_cntrmask, op_cntrmask));     // cntrmask roll
	ROLL_FALL(hvlineto_roll(il, j));  // hlineto-vlineto roll
	ROLL_FALL(hhvvcurve_roll(il, j)); // hhcurveto-vvcurveto roll
	ROLL_FALL(hvvhcurve_roll(il, j)); // hvcurveto-vhcurveto roll
	ROLL_FALL(nextstop(il, j));       // move to next stop for operand match
	return 1;                         // nothing match
}

void cff_optimizeIL(cff_CharstringIL *il, const otfcc_Options *options) {
	if (!options->cff_rollCharString) return;
	uint32_t j = 0;
	while (j < il->length) {
		j += decideAdvance(il, j, options->cff_rollCharString);
	}
}

// IL to buffer conversion
caryll_Buffer *cff_build_IL(cff_CharstringIL *il) {
	caryll_Buffer *blob = bufnew();

	for (uint16_t j = 0; j < il->length; j++) {
		switch (il->instr[j].type) {
			case IL_ITEM_OPERAND: {
				cff_mergeCS2Operand(blob, il->instr[j].d);
				break;
			}
			case IL_ITEM_OPERATOR: {
				cff_mergeCS2Operator(blob, il->instr[j].i);
				break;
			}
			case IL_ITEM_SPECIAL: {
				cff_mergeCS2Special(blob, il->instr[j].i);
				break;
			}
			default:
				break;
		}
	}
	return blob;
}

cff_CharstringIL *cff_shrinkIL(cff_CharstringIL *il) {
	cff_CharstringIL *out;
	NEW(out);
	for (uint16_t j = 0; j < il->length; j++) {
		switch (il->instr[j].type) {
			case IL_ITEM_OPERAND: {
				il_push_operand(out, il->instr[j].d);
				break;
			}
			case IL_ITEM_OPERATOR: {
				il_push_op(out, il->instr[j].i);
				break;
			}
			case IL_ITEM_SPECIAL: {
				il_push_special(out, il->instr[j].i);
				break;
			}
			default:
				break;
		}
	}
	return out;
}

void cff_ILmergeIL(cff_CharstringIL *self, cff_CharstringIL *il) {
	for (uint16_t j = 0; j < il->length; j++) {
		switch (il->instr[j].type) {
			case IL_ITEM_OPERAND: {
				il_push_operand(self, il->instr[j].d);
				break;
			}
			case IL_ITEM_OPERATOR: {
				il_push_op(self, il->instr[j].i);
				break;
			}
			case IL_ITEM_SPECIAL: {
				il_push_special(self, il->instr[j].i);
				break;
			}
			default:
				break;
		}
	}
}

bool instruction_eq(cff_CharstringInstruction *z1, cff_CharstringInstruction *z2) {
	if (z1->type == z2->type) {
		if (z1->type == IL_ITEM_OPERAND || z1->type == IL_ITEM_PHANTOM_OPERAND) {
			return z1->d == z2->d;
		} else {
			return z1->i == z2->i;
		}
	} else {
		return false;
	}
}

bool cff_ilEqual(cff_CharstringIL *a, cff_CharstringIL *b) {
	if (!a || !b) return false;
	if (a->length != b->length) return false;
	for (uint32_t j = 0; j < a->length; j++)
		if (!instruction_eq(a->instr + j, b->instr + j)) { return false; }
	return true;
}
