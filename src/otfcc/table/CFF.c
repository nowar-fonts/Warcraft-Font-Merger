#include "CFF.h"

#include "support/util.h"
#include "libcff/libcff.h"
#include "libcff/charstring-il.h"
#include "libcff/subr.h"

const double DEFAULT_BLUE_SCALE = 0.039625;
const double DEFAULT_BLUE_SHIFT = 7;
const double DEFAULT_BLUE_FUZZ = 1;
const double DEFAULT_EXPANSION_FACTOR = 0.06;

static cff_PrivateDict *otfcc_newCff_private() {
	cff_PrivateDict *pd = NULL;
	NEW(pd);
	pd->blueFuzz = DEFAULT_BLUE_FUZZ;
	pd->blueScale = DEFAULT_BLUE_SCALE;
	pd->blueShift = DEFAULT_BLUE_SHIFT;
	pd->expansionFactor = DEFAULT_EXPANSION_FACTOR;
	return pd;
}

static INLINE void initFD(table_CFF *fd) {
	memset(fd, 0, sizeof(*fd));
	fd->underlinePosition = -100;
	fd->underlineThickness = 50;
}
static void otfcc_delete_privatedict(cff_PrivateDict *priv) {
	if (!priv) return;
	FREE(priv->blueValues);
	FREE(priv->otherBlues);
	FREE(priv->familyBlues);

	FREE(priv->familyOtherBlues);
	FREE(priv->stemSnapH);
	FREE(priv->stemSnapV);
	FREE(priv);
}
static INLINE void disposeFontMatrix(cff_FontMatrix *fm) {
	if (!fm) return;
	iVQ.dispose(&fm->x);
	iVQ.dispose(&fm->y);
}
static INLINE void disposeFD(table_CFF *fd) {
	sdsfree(fd->version);
	sdsfree(fd->notice);
	sdsfree(fd->copyright);
	sdsfree(fd->fullName);
	sdsfree(fd->familyName);
	sdsfree(fd->weight);

	sdsfree(fd->fontName);
	sdsfree(fd->cidRegistry);
	sdsfree(fd->cidOrdering);
	disposeFontMatrix(fd->fontMatrix);
	FREE(fd->fontMatrix);
	otfcc_delete_privatedict(fd->privateDict);
	if (fd->fdArray) {
		for (tableid_t j = 0; j < fd->fdArrayCount; j++) {
			table_iCFF.free(fd->fdArray[j]);
		}
		FREE(fd->fdArray);
	}
}

caryll_standardRefType(table_CFF, table_iCFF, initFD, disposeFD);

typedef struct {
	int32_t fdArrayIndex;
	table_CFF *meta;
	table_glyf *glyphs;
	cff_File *cffFile;
	uint64_t seed;
} cff_extract_context;
static void callback_extract_private(uint32_t op, uint8_t top, cff_Value *stack, void *_context) {
	cff_extract_context *context = (cff_extract_context *)_context;
	table_CFF *meta = context->meta;
	if (context->fdArrayIndex >= 0 && context->fdArrayIndex < meta->fdArrayCount) {
		meta = meta->fdArray[context->fdArrayIndex];
	}
	cff_PrivateDict *pd = meta->privateDict;
	switch (op) {
		// DELTAs
		case op_BlueValues: {
			pd->blueValuesCount = top;
			NEW(pd->blueValues, pd->blueValuesCount);
			for (arity_t j = 0; j < pd->blueValuesCount; j++) {
				pd->blueValues[j] = cffnum(stack[j]);
			}
			break;
		}
		case op_OtherBlues: {
			pd->otherBluesCount = top;
			NEW(pd->otherBlues, pd->otherBluesCount);
			for (arity_t j = 0; j < pd->otherBluesCount; j++) {
				pd->otherBlues[j] = cffnum(stack[j]);
			}
			break;
		}
		case op_FamilyBlues: {
			pd->familyBluesCount = top;
			NEW(pd->familyBlues, pd->familyBluesCount);
			for (arity_t j = 0; j < pd->familyBluesCount; j++) {
				pd->familyBlues[j] = cffnum(stack[j]);
			}
			break;
		}
		case op_FamilyOtherBlues: {
			pd->familyOtherBluesCount = top;
			NEW(pd->familyOtherBlues, pd->familyOtherBluesCount);
			for (arity_t j = 0; j < pd->familyOtherBluesCount; j++) {
				pd->familyOtherBlues[j] = cffnum(stack[j]);
			}
			break;
		}
		case op_StemSnapH: {
			pd->stemSnapHCount = top;
			NEW(pd->stemSnapH, pd->stemSnapHCount);
			for (arity_t j = 0; j < pd->stemSnapHCount; j++) {
				pd->stemSnapH[j] = cffnum(stack[j]);
			}
			break;
		}
		case op_StemSnapV: {
			pd->stemSnapVCount = top;
			NEW(pd->stemSnapV, pd->stemSnapVCount);
			for (arity_t j = 0; j < pd->stemSnapVCount; j++) {
				pd->stemSnapV[j] = cffnum(stack[j]);
			}
			break;
		}
		// Numbers
		case op_BlueScale:
			if (top) { pd->blueScale = cffnum(stack[top - 1]); }
			break;
		case op_BlueShift:
			if (top) { pd->blueShift = cffnum(stack[top - 1]); }
			break;
		case op_BlueFuzz:
			if (top) { pd->blueFuzz = cffnum(stack[top - 1]); }
			break;
		case op_StdHW:
			if (top) { pd->stdHW = cffnum(stack[top - 1]); }
			break;
		case op_StdVW:
			if (top) { pd->stdVW = cffnum(stack[top - 1]); }
			break;
		case op_ForceBold:
			if (top) { pd->forceBold = cffnum(stack[top - 1]); }
			break;
		case op_LanguageGroup:
			if (top) { pd->languageGroup = cffnum(stack[top - 1]); }
			break;
		case op_ExpansionFactor:
			if (top) { pd->expansionFactor = cffnum(stack[top - 1]); }
			break;
		case op_initialRandomSeed:
			if (top) { pd->initialRandomSeed = cffnum(stack[top - 1]); }
			break;
		case op_defaultWidthX:
			if (top) { pd->defaultWidthX = cffnum(stack[top - 1]); }
			break;
		case op_nominalWidthX:
			if (top) { pd->nominalWidthX = cffnum(stack[top - 1]); }
			break;
	}
}

static void callback_extract_fd(uint32_t op, uint8_t top, cff_Value *stack, void *_context) {
	cff_extract_context *context = (cff_extract_context *)_context;
	cff_File *file = context->cffFile;
	table_CFF *meta = context->meta;
	if (context->fdArrayIndex >= 0 && context->fdArrayIndex < meta->fdArrayCount) {
		meta = meta->fdArray[context->fdArrayIndex];
	}
	switch (op) {
		case op_version:
			if (top) { meta->version = sdsget_cff_sid(stack[top - 1].i, file->string); }
			break;
		case op_Notice:
			if (top) { meta->notice = sdsget_cff_sid(stack[top - 1].i, file->string); }
			break;
		case op_Copyright:
			if (top) { meta->copyright = sdsget_cff_sid(stack[top - 1].i, file->string); }
			break;
		case op_FontName:
			if (top) { meta->fontName = sdsget_cff_sid(stack[top - 1].i, file->string); }
			break;
		case op_FullName:
			if (top) { meta->fullName = sdsget_cff_sid(stack[top - 1].i, file->string); }
			break;
		case op_FamilyName:
			if (top) { meta->familyName = sdsget_cff_sid(stack[top - 1].i, file->string); }
			break;
		case op_Weight:
			if (top) { meta->weight = sdsget_cff_sid(stack[top - 1].i, file->string); }
			break;
		case op_FontBBox:
			if (top >= 4) {
				meta->fontBBoxLeft = cffnum(stack[top - 4]);
				meta->fontBBoxBottom = cffnum(stack[top - 3]);
				meta->fontBBoxRight = cffnum(stack[top - 2]);
				meta->fontBBoxTop = cffnum(stack[top - 1]);
			}
			break;
		case op_FontMatrix:
			if (top >= 6) {
				NEW(meta->fontMatrix);
				meta->fontMatrix->a = cffnum(stack[top - 6]);
				meta->fontMatrix->b = cffnum(stack[top - 5]);
				meta->fontMatrix->c = cffnum(stack[top - 4]);
				meta->fontMatrix->d = cffnum(stack[top - 3]);
				meta->fontMatrix->x = iVQ.createStill(cffnum(stack[top - 2]));
				meta->fontMatrix->y = iVQ.createStill(cffnum(stack[top - 1]));
			}
			break;
		case op_isFixedPitch:
			if (top) { meta->isFixedPitch = (bool)cffnum(stack[top - 1]); }
			break;
		case op_ItalicAngle:
			if (top) { meta->italicAngle = cffnum(stack[top - 1]); }
			break;
		case op_UnderlinePosition:
			if (top) { meta->underlinePosition = cffnum(stack[top - 1]); }
			break;
		case op_UnderlineThickness:
			if (top) { meta->underlineThickness = cffnum(stack[top - 1]); }
			break;
		case op_StrokeWidth:
			if (top) { meta->strokeWidth = cffnum(stack[top - 1]); }
			break;

		// Private
		case op_Private:
			if (top >= 2) {
				uint32_t privateLength = cffnum(stack[top - 2]);
				uint32_t privateOffset = cffnum(stack[top - 1]);
				meta->privateDict = otfcc_newCff_private();
				cff_iDict.parseToCallback(file->raw_data + privateOffset, privateLength, context,
				                          callback_extract_private);
			}
			break;
		// CID
		case op_ROS:
			if (top >= 3) {
				meta->isCID = true;
				meta->cidRegistry = sdsget_cff_sid(stack[top - 3].i, file->string);
				meta->cidOrdering = sdsget_cff_sid(stack[top - 2].i, file->string);
				meta->cidSupplement = cffnum(stack[top - 1]);
			}
			break;
	}
}

typedef struct {
	glyf_Glyph *g;
	shapeid_t jContour;
	shapeid_t jPoint;
	double defaultWidthX;
	double nominalWidthX;
	uint8_t definedHStems;
	uint8_t definedVStems;
	uint8_t definedHintMasks;
	uint8_t definedContourMasks;
	uint64_t randx;
} outline_builder_context;

static void callback_draw_setwidth(void *_context, double width) {
	outline_builder_context *context = (outline_builder_context *)_context;
	iVQ.replace(&context->g->advanceWidth, iVQ.createStill(width + context->nominalWidthX));
}
static void callback_draw_next_contour(void *_context) {
	outline_builder_context *context = (outline_builder_context *)_context;
	glyf_Contour c;
	glyf_iContour.init(&c);
	glyf_iContourList.push(&context->g->contours, c);
	context->jContour = context->g->contours.length;
	context->jPoint = 0;
}
static void callback_draw_lineto(void *_context, double x1, double y1) {
	outline_builder_context *context = (outline_builder_context *)_context;
	if (context->jContour) {
		glyf_Contour *contour = &context->g->contours.items[context->jContour - 1];
		glyf_Point z;
		glyf_iPoint.init(&z);
		z.onCurve = true;
		iVQ.copyReplace(&z.x, iVQ.createStill(x1));
		iVQ.copyReplace(&z.y, iVQ.createStill(y1));
		glyf_iContour.push(contour, z);
		context->jPoint += 1;
	}
}
static void callback_draw_curveto(void *_context, double x1, double y1, double x2, double y2,
                                  double x3, double y3) {
	outline_builder_context *context = (outline_builder_context *)_context;
	if (context->jContour) {
		glyf_Contour *contour = &context->g->contours.items[context->jContour - 1];
		{
			glyf_Point z;
			glyf_iPoint.init(&z);
			z.onCurve = false;
			iVQ.copyReplace(&z.x, iVQ.createStill(x1));
			iVQ.copyReplace(&z.y, iVQ.createStill(y1));
			glyf_iContour.push(contour, z);
		}
		{
			glyf_Point z;
			glyf_iPoint.init(&z);
			z.onCurve = false;
			iVQ.copyReplace(&z.x, iVQ.createStill(x2));
			iVQ.copyReplace(&z.y, iVQ.createStill(y2));
			glyf_iContour.push(contour, z);
		}
		{
			glyf_Point z;
			glyf_iPoint.init(&z);
			z.onCurve = true;
			iVQ.copyReplace(&z.x, iVQ.createStill(x3));
			iVQ.copyReplace(&z.y, iVQ.createStill(y3));
			glyf_iContour.push(contour, z);
		}
		context->jPoint += 3;
	}
}
static void callback_draw_sethint(void *_context, bool isVertical, double position, double width) {
	outline_builder_context *context = (outline_builder_context *)_context;
	glyf_iStemDefList.push((isVertical ? &context->g->stemV : &context->g->stemH), //
	                       ((glyf_PostscriptStemDef){
	                           .position = position,
	                           .width = width,
	                       }));
}
static void callback_draw_setmask(void *_context, bool isContourMask, bool *maskArray) {
	outline_builder_context *context = (outline_builder_context *)_context;
	glyf_MaskList *maskList = isContourMask ? &context->g->contourMasks : &context->g->hintMasks;
	glyf_PostscriptHintMask mask;

	if (context->jContour) {
		mask.contoursBefore = context->jContour - 1;
	} else {
		mask.contoursBefore = 0;
	}
	mask.pointsBefore = context->jPoint;

	for (shapeid_t j = 0; j < 0x100; j++) {
		mask.maskH[j] = j < context->g->stemH.length ? maskArray[j] : 0;
		mask.maskV[j] = j < context->g->stemV.length ? maskArray[j + context->g->stemH.length] : 0;
	}

	FREE(maskArray);
	if (maskList->length > 0 &&
	    maskList->items[maskList->length - 1].contoursBefore == mask.contoursBefore &&
	    maskList->items[maskList->length - 1].pointsBefore == mask.pointsBefore) {
		// two masks are stacking together
		// simply replace maskH and maskV
		for (shapeid_t j = 0; j < 0x100; j++) {
			maskList->items[maskList->length - 1].maskH[j] = mask.maskH[j];
			maskList->items[maskList->length - 1].maskV[j] = mask.maskV[j];
		}
	} else {
		glyf_iMaskList.push(maskList, mask);
		if (isContourMask) {
			context->definedContourMasks += 1;
		} else {
			context->definedHintMasks += 1;
		}
	}
}

static double callback_draw_getrand(void *_context) {
	// xorshift64* PRNG to double53
	outline_builder_context *context = (outline_builder_context *)_context;
	uint64_t x = context->randx;
	x ^= x >> 12;
	x ^= x << 25;
	x ^= x >> 27;
	context->randx = x;
	union {
		uint64_t u;
		double d;
	} a;
	a.u = x * UINT64_C(2685821657736338717);
	a.u = (a.u >> 12) | UINT64_C(0x3FF0000000000000);
	double q = (a.u & 2048) ? (1.0 - (2.2204460492503131E-16 / 2.0)) : 1.0;
	return a.d - q;
}

static cff_IOutlineBuilder drawPass = {.setWidth = callback_draw_setwidth,
                                       .newContour = callback_draw_next_contour,
                                       .lineTo = callback_draw_lineto,
                                       .curveTo = callback_draw_curveto,
                                       .setHint = callback_draw_sethint,
                                       .setMask = callback_draw_setmask,
                                       .getrand = callback_draw_getrand};

static void buildOutline(glyphid_t i, cff_extract_context *context, const otfcc_Options *options) {
	cff_File *f = context->cffFile;
	glyf_Glyph *g = otfcc_newGlyf_glyph();
	context->glyphs->items[i] = g;
	uint64_t seed = context->seed;

	cff_Index localSubrs;
	cff_iIndex.init(&localSubrs);

	cff_Stack stack;
	stack.max = 0x10000;
	NEW(stack.stack, stack.max);
	stack.index = 0;
	stack.stem = 0;

	outline_builder_context bc = {g, 0, 0, 0.0, 0.0, 0, 0, 0, 0, 0};

	// Determine used FD index and subroutine list
	uint8_t fd = 0;
	if (f->fdselect.t != cff_FDSELECT_UNSPECED)
		fd = cff_parseSubr(i, f->raw_data, f->font_dict, f->fdselect, &localSubrs);
	else
		fd = cff_parseSubr(i, f->raw_data, f->top_dict, f->fdselect, &localSubrs);

	g->fdSelect = Handle.fromIndex(fd);

	// Decide default and nominal width
	if (context->meta->fdArray && fd < context->meta->fdArrayCount &&
	    context->meta->fdArray[fd]->privateDict) {
		bc.defaultWidthX = context->meta->fdArray[fd]->privateDict->defaultWidthX;
		bc.nominalWidthX = context->meta->fdArray[fd]->privateDict->nominalWidthX;
	} else if (context->meta->privateDict) {
		bc.defaultWidthX = context->meta->privateDict->defaultWidthX;
		bc.nominalWidthX = context->meta->privateDict->nominalWidthX;
	}
	iVQ.replace(&g->advanceWidth, iVQ.createStill(bc.defaultWidthX));

	// Get charstring
	uint8_t *charStringPtr = f->char_strings.data + f->char_strings.offset[i] - 1;
	uint32_t charStringLength = f->char_strings.offset[i + 1] - f->char_strings.offset[i];

	// Draw points
	stack.index = 0;
	stack.stem = 0;
	bc.jContour = 0;
	bc.jPoint = 0;
	bc.randx = seed;

	cff_parseOutline(charStringPtr, charStringLength, f->global_subr, localSubrs, &stack, &bc,
	                 drawPass, options);

	// Turn deltas into absolute coordinates
	VQ cx = iVQ.neutral(), cy = iVQ.neutral();
	for (shapeid_t j = 0; j < g->contours.length; j++) {
		glyf_Contour *contour = &g->contours.items[j];
		for (shapeid_t k = 0; k < contour->length; k++) {
			glyf_Point *z = &contour->items[k];
			iVQ.inplacePlus(&cx, z->x);
			iVQ.inplacePlus(&cy, z->y);
			iVQ.copyReplace(&z->x, cx);
			iVQ.copyReplace(&z->y, cy);
		}
		if (!iVQ.compare(contour->items[0].x, contour->items[contour->length - 1].x) &&
		    !iVQ.compare(contour->items[0].y, contour->items[contour->length - 1].y) &&
		    (contour->items[0].onCurve && contour->items[contour->length - 1].onCurve)) {
			// We found a duplicate knot at the beginning and end of a curve
			// mainly due to curveto. We can remove that. This is unnecessary.
			glyf_iContour.pop(contour);
		}
		glyf_iContour.shrinkToFit(contour);
	}
	glyf_iContourList.shrinkToFit(&g->contours);
	iVQ.dispose(&cx), iVQ.dispose(&cy);

	cff_iIndex.dispose(&localSubrs);
	FREE(stack.stack);
	context->seed = bc.randx;
}

static sds formCIDString(cffsid_t cid) {
	return sdscatprintf(sdsnew("CID"), "%d", cid);
}

static void nameGlyphsAccordingToCFF(cff_extract_context *context) {
	cff_File *cffFile = context->cffFile;
	table_glyf *glyphs = context->glyphs;
	cff_Charset *charset = &cffFile->charsets;
	if (context->meta->isCID) {
		switch (charset->t) {
			case cff_CHARSET_FORMAT0: {
				for (glyphid_t j = 0; j < charset->s; j++) {
					cffsid_t sid = charset->f0.glyph[j];
					sds glyphname = sdsget_cff_sid(sid, cffFile->string);
					if (glyphname) {
						glyphs->items[j + 1]->name = glyphname;
						glyphs->items[j + 1]->cid = sid;
					}
				}
				break;
			}
			case cff_CHARSET_FORMAT1: {
				uint32_t glyphsNamedSofar = 1;
				for (glyphid_t j = 0; j < charset->s; j++) {
					cffsid_t first = charset->f1.range1[j].first;
					for (glyphid_t k = 0; k <= charset->f1.range1[j].nleft; k++) {
						cffsid_t sid = first + k;
						sds glyphname = formCIDString(sid);
						if (glyphsNamedSofar < glyphs->length && glyphname) {
							glyphs->items[glyphsNamedSofar]->name = glyphname;
							glyphs->items[glyphsNamedSofar]->cid = sid;
						}
						glyphsNamedSofar++;
					}
				}
				break;
			}
			case cff_CHARSET_FORMAT2: {
				uint32_t glyphsNamedSofar = 1;
				for (glyphid_t j = 0; j < charset->s; j++) {
					cffsid_t first = charset->f2.range2[j].first;
					for (glyphid_t k = 0; k <= charset->f2.range2[j].nleft; k++) {
						cffsid_t sid = first + k;
						sds glyphname = formCIDString(sid);
						if (glyphsNamedSofar < glyphs->length && glyphname) {
							glyphs->items[glyphsNamedSofar]->name = glyphname;
							glyphs->items[glyphsNamedSofar]->cid = sid;
						}
						glyphsNamedSofar++;
					}
				}
				break;
			}
		}
	} else {
		switch (charset->t) {
			case cff_CHARSET_FORMAT0: {
				for (glyphid_t j = 0; j < charset->s; j++) {
					cffsid_t sid = charset->f0.glyph[j];
					sds glyphname = sdsget_cff_sid(sid, cffFile->string);
					if (glyphname) { glyphs->items[j + 1]->name = glyphname; }
				}
				break;
			}
			case cff_CHARSET_FORMAT1: {
				uint32_t glyphsNamedSofar = 1;
				for (glyphid_t j = 0; j < charset->s; j++) {
					glyphid_t first = charset->f1.range1[j].first;
					for (glyphid_t k = 0; k <= charset->f1.range1[j].nleft; k++) {
						cffsid_t sid = first + k;
						sds glyphname = sdsget_cff_sid(sid, cffFile->string);
						if (glyphsNamedSofar < glyphs->length && glyphname) {
							glyphs->items[glyphsNamedSofar]->name = glyphname;
						}
						glyphsNamedSofar++;
					}
				}
				break;
			}
			case cff_CHARSET_FORMAT2: {
				uint32_t glyphsNamedSofar = 1;
				for (glyphid_t j = 0; j < charset->s; j++) {
					glyphid_t first = charset->f2.range2[j].first;
					for (glyphid_t k = 0; k <= charset->f2.range2[j].nleft; k++) {
						cffsid_t sid = first + k;
						sds glyphname = sdsget_cff_sid(sid, cffFile->string);
						if (glyphsNamedSofar < glyphs->length && glyphname) {
							glyphs->items[glyphsNamedSofar]->name = glyphname;
						}
						glyphsNamedSofar++;
					}
				}
				break;
			}
		}
	}
}

static double qround(const double x) {
	return otfcc_from_fixed(otfcc_to_fixed(x));
}
static void applyCffMatrix(table_CFF *CFF_, table_glyf *glyf, const table_head *head) {
	for (glyphid_t jj = 0; jj < glyf->length; jj++) {
		glyf_Glyph *g = glyf->items[jj];
		table_CFF *fd = CFF_;
		if (fd->fdArray && g->fdSelect.index < fd->fdArrayCount) {
			fd = fd->fdArray[g->fdSelect.index];
		}
		if (fd->fontMatrix) {
			scale_t a = qround(head->unitsPerEm * fd->fontMatrix->a);
			scale_t b = qround(head->unitsPerEm * fd->fontMatrix->b);
			scale_t c = qround(head->unitsPerEm * fd->fontMatrix->c);
			scale_t d = qround(head->unitsPerEm * fd->fontMatrix->d);
			VQ x = iVQ.scale(fd->fontMatrix->x, head->unitsPerEm);
			x.kernel = qround(x.kernel);
			VQ y = iVQ.scale(fd->fontMatrix->y, head->unitsPerEm);
			y.kernel = qround(y.kernel);
			for (shapeid_t j = 0; j < g->contours.length; j++) {
				glyf_Contour *contour = &g->contours.items[j];
				for (shapeid_t k = 0; k < contour->length; k++) {
					VQ zx = iVQ.dup(contour->items[k].x);
					VQ zy = iVQ.dup(contour->items[k].y);
					iVQ.replace(&contour->items[k].x, iVQ.pointLinearTfm(x, a, zx, b, zy));
					iVQ.replace(&contour->items[k].y, iVQ.pointLinearTfm(y, c, zx, d, zy));
					iVQ.dispose(&zx), iVQ.dispose(&zy);
				}
			}
			iVQ.dispose(&x), iVQ.dispose(&y);
		}
	}
}

table_CFFAndGlyf otfcc_readCFFAndGlyfTables(const otfcc_Packet packet, const otfcc_Options *options,
                                            const table_head *head) {
	table_CFFAndGlyf ret;
	ret.meta = NULL;
	ret.glyphs = NULL;

	cff_extract_context context;
	context.fdArrayIndex = -1;
	context.meta = NULL;
	context.glyphs = NULL;
	context.cffFile = NULL;
	FOR_TABLE('CFF ', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		cff_File *cffFile = cff_openStream(data, length, options);
		context.cffFile = cffFile;
		context.meta = table_iCFF.create();

		// Extract data in TOP DICT
		cff_iDict.parseToCallback(cffFile->top_dict.data,
		                          cffFile->top_dict.offset[1] - cffFile->top_dict.offset[0],
		                          &context, callback_extract_fd);

		if (!context.meta->fontName) {
			context.meta->fontName = sdsget_cff_sid(391, cffFile->name);
		}

		// We have FDArray
		if (cffFile->font_dict.count) {
			context.meta->fdArrayCount = cffFile->font_dict.count;
			NEW(context.meta->fdArray, context.meta->fdArrayCount);
			for (tableid_t j = 0; j < context.meta->fdArrayCount; j++) {
				context.meta->fdArray[j] = table_iCFF.create();
				context.fdArrayIndex = j;
				cff_iDict.parseToCallback(
				    cffFile->font_dict.data + cffFile->font_dict.offset[j] - 1,
				    cffFile->font_dict.offset[j + 1] - cffFile->font_dict.offset[j], &context,
				    callback_extract_fd);
				if (!context.meta->fdArray[j]->fontName) {
					context.meta->fdArray[j]->fontName = sdscatprintf(sdsempty(), "_Subfont%d", j);
				}
			}
		}
		ret.meta = context.meta;

		// Extract data of outlines
		context.seed = 0x1234567887654321;
		if (context.meta->privateDict) {
			context.seed =
			    (uint64_t)context.meta->privateDict->initialRandomSeed ^ 0x1234567887654321;
		}
		table_glyf *glyphs = table_iGlyf.createN(cffFile->char_strings.count);
		context.glyphs = glyphs;
		for (glyphid_t j = 0; j < glyphs->length; j++) {
			buildOutline(j, &context, options);
		}

		applyCffMatrix(context.meta, context.glyphs, head);

		// Name glyphs according charset
		nameGlyphsAccordingToCFF(&context);

		ret.glyphs = context.glyphs;
		cff_close(cffFile);
	}
	return ret;
}

static void pdDeltaToJson(json_value *target, const char *field, arity_t count, double *values) {
	if (!count || !values) return;
	json_value *a = json_array_new(count);
	for (arity_t j = 0; j < count; j++) {
		json_array_push(a, json_double_new(values[j]));
	}
	json_object_push(target, field, a);
}

static json_value *pdToJson(const cff_PrivateDict *pd) {
	json_value *_pd = json_object_new(24);
	pdDeltaToJson(_pd, "blueValues", pd->blueValuesCount, pd->blueValues);
	pdDeltaToJson(_pd, "otherBlues", pd->otherBluesCount, pd->otherBlues);
	pdDeltaToJson(_pd, "familyBlues", pd->familyBluesCount, pd->familyBlues);
	pdDeltaToJson(_pd, "familyOtherBlues", pd->familyOtherBluesCount, pd->familyOtherBlues);
	pdDeltaToJson(_pd, "stemSnapH", pd->stemSnapHCount, pd->stemSnapH);
	pdDeltaToJson(_pd, "stemSnapV", pd->stemSnapVCount, pd->stemSnapV);
	if (pd->blueScale != DEFAULT_BLUE_SCALE)
		json_object_push(_pd, "blueScale", json_double_new(pd->blueScale));
	if (pd->blueShift != DEFAULT_BLUE_SHIFT)
		json_object_push(_pd, "blueShift", json_double_new(pd->blueShift));
	if (pd->blueFuzz != DEFAULT_BLUE_FUZZ)
		json_object_push(_pd, "blueFuzz", json_double_new(pd->blueFuzz));
	if (pd->stdHW) json_object_push(_pd, "stdHW", json_double_new(pd->stdHW));
	if (pd->stdVW) json_object_push(_pd, "stdVW", json_double_new(pd->stdVW));
	if (pd->forceBold) json_object_push(_pd, "forceBold", json_boolean_new(pd->forceBold));
	if (pd->languageGroup)
		json_object_push(_pd, "languageGroup", json_double_new(pd->languageGroup));
	if (pd->expansionFactor != DEFAULT_EXPANSION_FACTOR)
		json_object_push(_pd, "expansionFactor", json_double_new(pd->expansionFactor));
	if (pd->initialRandomSeed)
		json_object_push(_pd, "initialRandomSeed", json_double_new(pd->initialRandomSeed));
	if (pd->defaultWidthX)
		json_object_push(_pd, "defaultWidthX", json_double_new(pd->defaultWidthX));
	if (pd->nominalWidthX)
		json_object_push(_pd, "nominalWidthX", json_double_new(pd->nominalWidthX));
	return _pd;
}
static json_value *fdToJson(const table_CFF *table) {
	json_value *_CFF_ = json_object_new(24);

	if (table->isCID) json_object_push(_CFF_, "isCID", json_boolean_new(table->isCID));

	if (table->version) json_object_push(_CFF_, "version", json_from_sds(table->version));
	if (table->notice) json_object_push(_CFF_, "notice", json_from_sds(table->notice));
	if (table->copyright) json_object_push(_CFF_, "copyright", json_from_sds(table->copyright));
	if (table->fontName) json_object_push(_CFF_, "fontName", json_from_sds(table->fontName));
	if (table->fullName) json_object_push(_CFF_, "fullName", json_from_sds(table->fullName));
	if (table->familyName) json_object_push(_CFF_, "familyName", json_from_sds(table->familyName));
	if (table->weight) json_object_push(_CFF_, "weight", json_from_sds(table->weight));

	if (table->isFixedPitch)
		json_object_push(_CFF_, "isFixedPitch", json_boolean_new(table->isFixedPitch));
	if (table->italicAngle)
		json_object_push(_CFF_, "italicAngle", json_double_new(table->italicAngle));
	if (table->underlinePosition != -100)
		json_object_push(_CFF_, "underlinePosition", json_double_new(table->underlinePosition));
	if (table->underlineThickness != 50)
		json_object_push(_CFF_, "underlineThickness", json_double_new(table->underlineThickness));
	if (table->strokeWidth)
		json_object_push(_CFF_, "strokeWidth", json_double_new(table->strokeWidth));
	if (table->fontBBoxLeft)
		json_object_push(_CFF_, "fontBBoxLeft", json_double_new(table->fontBBoxLeft));
	if (table->fontBBoxBottom)
		json_object_push(_CFF_, "fontBBoxBottom", json_double_new(table->fontBBoxBottom));
	if (table->fontBBoxRight)
		json_object_push(_CFF_, "fontBBoxRight", json_double_new(table->fontBBoxRight));
	if (table->fontBBoxTop)
		json_object_push(_CFF_, "fontBBoxTop", json_double_new(table->fontBBoxTop));

	if (table->fontMatrix) {
		json_value *_fontMatrix = json_object_new(6);
		json_object_push(_fontMatrix, "a", json_double_new(table->fontMatrix->a));
		json_object_push(_fontMatrix, "b", json_double_new(table->fontMatrix->b));
		json_object_push(_fontMatrix, "c", json_double_new(table->fontMatrix->c));
		json_object_push(_fontMatrix, "d", json_double_new(table->fontMatrix->d));
		json_object_push(_fontMatrix, "x", json_new_VQ(table->fontMatrix->x, NULL));
		json_object_push(_fontMatrix, "y", json_new_VQ(table->fontMatrix->y, NULL));
		json_object_push(_CFF_, "fontMatrix", _fontMatrix);
	}
	if (table->privateDict) { json_object_push(_CFF_, "privates", pdToJson(table->privateDict)); }

	if (table->cidRegistry && table->cidOrdering) {
		json_object_push(_CFF_, "cidRegistry", json_from_sds(table->cidRegistry));
		json_object_push(_CFF_, "cidOrdering", json_from_sds(table->cidOrdering));
		json_object_push(_CFF_, "cidSupplement", json_integer_new(table->cidSupplement));
	}
	if (table->fdArray) {
		json_value *_fdArray = json_object_new(table->fdArrayCount);
		for (tableid_t j = 0; j < table->fdArrayCount; j++) {
			sds name = table->fdArray[j]->fontName;
			table->fdArray[j]->fontName = NULL;
			json_object_push(_fdArray, name, fdToJson(table->fdArray[j]));
			table->fdArray[j]->fontName = name;
		}
		json_object_push(_CFF_, "fdArray", _fdArray);
	}
	return _CFF_;
}

void otfcc_dumpCFF(const table_CFF *table, json_value *root, const otfcc_Options *options) {
	if (!table) return;
	loggedStep("CFF") {
		json_object_push(root, "CFF_", fdToJson(table));
	}
}

static void pdDeltaFromJson(json_value *dump, arity_t *count, double **array) {
	if (!dump || dump->type != json_array) return;
	*count = dump->u.array.length;
	NEW(*array, *count);
	for (arity_t j = 0; j < *count; j++) {
		(*array)[j] = json_numof(dump->u.array.values[j]);
	}
}
static cff_PrivateDict *pdFromJson(json_value *dump) {
	if (!dump || dump->type != json_object) return NULL;
	cff_PrivateDict *pd = otfcc_newCff_private();
	pdDeltaFromJson(json_obj_get(dump, "blueValues"), &(pd->blueValuesCount), &(pd->blueValues));
	pdDeltaFromJson(json_obj_get(dump, "otherBlues"), &(pd->otherBluesCount), &(pd->otherBlues));
	pdDeltaFromJson(json_obj_get(dump, "familyBlues"), &(pd->familyBluesCount), &(pd->familyBlues));
	pdDeltaFromJson(json_obj_get(dump, "familyOtherBlues"), &(pd->familyOtherBluesCount),
	                &(pd->familyOtherBlues));
	pdDeltaFromJson(json_obj_get(dump, "stemSnapH"), &(pd->stemSnapHCount), &(pd->stemSnapH));
	pdDeltaFromJson(json_obj_get(dump, "stemSnapV"), &(pd->stemSnapVCount), &(pd->stemSnapV));

	pd->blueScale = json_obj_getnum_fallback(dump, "blueScale", DEFAULT_BLUE_SCALE);
	pd->blueShift = json_obj_getnum_fallback(dump, "blueShift", DEFAULT_BLUE_SHIFT);
	pd->blueFuzz = json_obj_getnum_fallback(dump, "blueFuzz", DEFAULT_BLUE_FUZZ);
	pd->stdHW = json_obj_getnum(dump, "stdHW");
	pd->stdVW = json_obj_getnum(dump, "stdVW");
	pd->forceBold = json_obj_getbool(dump, "forceBold");
	pd->languageGroup = json_obj_getnum(dump, "languageGroup");
	pd->expansionFactor =
	    json_obj_getnum_fallback(dump, "expansionFactor", DEFAULT_EXPANSION_FACTOR);
	pd->initialRandomSeed = json_obj_getnum(dump, "initialRandomSeed");
	// Not used -- they will be automatically calculated
	// pd->defaultWidthX = json_obj_getnum(dump, "defaultWidthX");
	// pd->nominalWidthX = json_obj_getnum(dump, "nominalWidthX");

	return pd;
}
static table_CFF *fdFromJson(const json_value *dump, const otfcc_Options *options, bool topLevel) {
	table_CFF *table = table_iCFF.create();
	if (!dump || dump->type != json_object) return table;
	// Names
	table->version = json_obj_getsds(dump, "version");
	table->notice = json_obj_getsds(dump, "notice");
	table->copyright = json_obj_getsds(dump, "copyright");
	table->fontName = json_obj_getsds(dump, "fontName");
	table->fullName = json_obj_getsds(dump, "fullName");
	table->familyName = json_obj_getsds(dump, "familyName");
	table->weight = json_obj_getsds(dump, "weight");

	// Metrics
	table->isFixedPitch = json_obj_getbool(dump, "isFixedPitch");
	table->italicAngle = json_obj_getnum(dump, "italicAngle");
	table->underlinePosition = json_obj_getnum_fallback(dump, "underlinePosition", -100.0);
	table->underlineThickness = json_obj_getnum_fallback(dump, "underlineThickness", 50.0);
	table->strokeWidth = json_obj_getnum(dump, "strokeWidth");
	table->fontBBoxLeft = json_obj_getnum(dump, "fontBBoxLeft");
	table->fontBBoxBottom = json_obj_getnum(dump, "fontBBoxBottom");
	table->fontBBoxRight = json_obj_getnum(dump, "fontBBoxRight");
	table->fontBBoxTop = json_obj_getnum(dump, "fontBBoxTop");

	// fontMatrix
	// Need?
	/*
	json_value *fmatdump = json_obj_get_type(dump, "fontMatrix", json_object);
	if (fmatdump) {
	    NEW(table->fontMatrix);
	    table->fontMatrix->a = json_obj_getnum(fmatdump, "a");
	    table->fontMatrix->b = json_obj_getnum(fmatdump, "b");
	    table->fontMatrix->c = json_obj_getnum(fmatdump, "c");
	    table->fontMatrix->d = json_obj_getnum(fmatdump, "d");
	    table->fontMatrix->x = json_obj_getnum(fmatdump, "x");
	    table->fontMatrix->y = json_obj_getnum(fmatdump, "y");
	}
	*/

	// privates
	table->privateDict = pdFromJson(json_obj_get_type(dump, "privates", json_object));

	// CID
	table->cidRegistry = json_obj_getsds(dump, "cidRegistry");
	table->cidOrdering = json_obj_getsds(dump, "cidOrdering");
	table->cidSupplement = json_obj_getint(dump, "cidSupplement");
	table->UIDBase = json_obj_getint(dump, "UIDBase");
	table->cidCount = json_obj_getint(dump, "cidCount");
	table->cidFontVersion = json_obj_getnum(dump, "cidFontVersion");
	table->cidFontRevision = json_obj_getnum(dump, "cidFontRevision");

	// fdArray
	json_value *fdarraydump = json_obj_get_type(dump, "fdArray", json_object);
	if (fdarraydump) {
		table->isCID = true;
		table->fdArrayCount = fdarraydump->u.object.length;
		NEW(table->fdArray, table->fdArrayCount);
		for (tableid_t j = 0; j < table->fdArrayCount; j++) {
			table->fdArray[j] = fdFromJson(fdarraydump->u.object.values[j].value, options, false);
			if (table->fdArray[j]->fontName) { sdsfree(table->fdArray[j]->fontName); }
			table->fdArray[j]->fontName = sdsnewlen(fdarraydump->u.object.values[j].name,
			                                        fdarraydump->u.object.values[j].name_length);
		}
	}
	if (!table->fontName) table->fontName = sdsnew("CARYLL_CFFFONT");
	if (!table->privateDict) table->privateDict = otfcc_newCff_private();
	// Deal with force-cid
	if (topLevel && options->force_cid && !table->fdArray) {
		table->fdArrayCount = 1;
		NEW(table->fdArray, table->fdArrayCount);
		table->fdArray[0] = table_iCFF.create();
		table_CFF *fd0 = table->fdArray[0];
		fd0->privateDict = table->privateDict;
		table->privateDict = otfcc_newCff_private();
		fd0->fontName = sdscat(sdsdup(table->fontName), "-subfont0");
		table->isCID = true;
	}
	if (table->isCID && !table->cidRegistry) { table->cidRegistry = sdsnew("CARYLL"); }
	if (table->isCID && !table->cidOrdering) { table->cidOrdering = sdsnew("OTFCCAUTOCID"); }
	return table;
}
table_CFF *otfcc_parseCFF(const json_value *root, const otfcc_Options *options) {
	json_value *dump = json_obj_get_type(root, "CFF_", json_object);
	if (!dump) {
		return NULL;
	} else {
		table_CFF *cff = NULL;
		loggedStep("CFF") {
			cff = fdFromJson(dump, options, true);
		}
		return cff;
	}
}

typedef struct {
	table_glyf *glyf;
	uint16_t defaultWidth;
	uint16_t nominalWidthX;
	const otfcc_Options *options;
	cff_SubrGraph graph;
} cff_charstring_builder_context;
typedef struct {
	caryll_Buffer *charStrings;
	caryll_Buffer *subroutines;
} cff_charStringAndSubrs;

static void cff_make_charstrings(cff_charstring_builder_context *context, caryll_Buffer **s,
                                 caryll_Buffer **gs, caryll_Buffer **ls) {
	if (context->glyf->length == 0) { return; }
	for (glyphid_t j = 0; j < context->glyf->length; j++) {
		cff_CharstringIL *il = cff_compileGlyphToIL(context->glyf->items[j], context->defaultWidth,
		                                            context->nominalWidthX);
		cff_optimizeIL(il, context->options);
		cff_insertILToGraph(&context->graph, il);
		FREE(il->instr);
		FREE(il);
	}
	cff_ilGraphToBuffers(&context->graph, s, gs, ls, context->options);
}

// String table management
typedef struct {
	int sid;
	char *str;
	UT_hash_handle hh;
} cff_sid_entry;

static int sidof(cff_sid_entry **h, sds s) {
	cff_sid_entry *item = NULL;
	HASH_FIND_STR(*h, s, item);
	if (item) {
		return 391 + item->sid;
	} else {
		NEW(item);
		item->sid = HASH_COUNT(*h);
		item->str = sdsdup(s);
		HASH_ADD_STR(*h, str, item);
		return 391 + item->sid;
	}
}

static cff_DictEntry *cffdict_givemeablank(cff_Dict *dict) {
	dict->count++;
	RESIZE(dict->ents, dict->count);
	return &(dict->ents[dict->count - 1]);
}
static void cffdict_input(cff_Dict *dict, uint32_t op, cff_Value_Type t, arity_t arity, ...) {
	cff_DictEntry *last = cffdict_givemeablank(dict);
	last->op = op;
	last->cnt = arity;
	NEW(last->vals, arity);

	va_list ap;
	va_start(ap, arity);
	for (arity_t j = 0; j < arity; j++) {
		if (t == cff_DOUBLE) {
			double x = va_arg(ap, double);
			if (x == round(x)) {
				last->vals[j].t = cff_INTEGER;
				last->vals[j].i = round(x);
			} else {
				last->vals[j].t = cff_DOUBLE;
				last->vals[j].d = x;
			}
		} else {
			int x = va_arg(ap, int);
			last->vals[j].t = t;
			last->vals[j].i = x;
		}
	}
	va_end(ap);
}
static void cffdict_input_array(cff_Dict *dict, uint32_t op, cff_Value_Type t, arity_t arity,
                                double *arr) {
	if (!arity || !arr) return;
	cff_DictEntry *last = cffdict_givemeablank(dict);
	last->op = op;
	last->cnt = arity;
	NEW(last->vals, arity);
	for (arity_t j = 0; j < arity; j++) {
		double x = arr[j];
		if (t == cff_DOUBLE) {
			if (x == round(x)) {
				last->vals[j].t = cff_INTEGER;
				last->vals[j].i = round(x);
			} else {
				last->vals[j].t = cff_DOUBLE;
				last->vals[j].d = x;
			}
		} else {
			last->vals[j].t = t;
			last->vals[j].i = round(x);
		}
	}
}

static cff_Dict *cff_make_fd_dict(table_CFF *fd, cff_sid_entry **h) {
	cff_Dict *dict = cff_iDict.create();
	// ROS
	if (fd->cidRegistry && fd->cidOrdering) {
		cffdict_input(dict, op_ROS, cff_INTEGER, 3, sidof(h, fd->cidRegistry),
		              sidof(h, fd->cidOrdering), fd->cidSupplement);
	}

	// CFF Names
	if (fd->version) cffdict_input(dict, op_version, cff_INTEGER, 1, sidof(h, fd->version));
	if (fd->notice) cffdict_input(dict, op_Notice, cff_INTEGER, 1, sidof(h, fd->notice));
	if (fd->copyright) cffdict_input(dict, op_Copyright, cff_INTEGER, 1, sidof(h, fd->copyright));
	if (fd->fullName) cffdict_input(dict, op_FullName, cff_INTEGER, 1, sidof(h, fd->fullName));
	if (fd->familyName)
		cffdict_input(dict, op_FamilyName, cff_INTEGER, 1, sidof(h, fd->familyName));
	if (fd->weight) cffdict_input(dict, op_Weight, cff_INTEGER, 1, sidof(h, fd->weight));

	// CFF Metrics
	cffdict_input(dict, op_FontBBox, cff_DOUBLE, 4, fd->fontBBoxLeft, fd->fontBBoxBottom,
	              fd->fontBBoxRight, fd->fontBBoxTop);
	cffdict_input(dict, op_isFixedPitch, cff_INTEGER, 1, (int)fd->isFixedPitch);
	cffdict_input(dict, op_ItalicAngle, cff_DOUBLE, 1, fd->italicAngle);
	cffdict_input(dict, op_UnderlinePosition, cff_DOUBLE, 1, fd->underlinePosition);
	cffdict_input(dict, op_UnderlineThickness, cff_DOUBLE, 1, fd->underlineThickness);
	cffdict_input(dict, op_StrokeWidth, cff_DOUBLE, 1, fd->strokeWidth);
	if (fd->fontMatrix) {
		cffdict_input(dict, op_FontMatrix, cff_DOUBLE, 6,
					  fd->fontMatrix->a, fd->fontMatrix->b,
		              fd->fontMatrix->c, fd->fontMatrix->d,
					  iVQ.getStill(fd->fontMatrix->x), iVQ.getStill(fd->fontMatrix->y));
	}

	// CID specific
	if (fd->fontName) cffdict_input(dict, op_FontName, cff_INTEGER, 1, sidof(h, fd->fontName));
	if (fd->cidFontVersion)
		cffdict_input(dict, op_CIDFontVersion, cff_DOUBLE, 1, fd->cidFontVersion);
	if (fd->cidFontRevision)
		cffdict_input(dict, op_CIDFontRevision, cff_DOUBLE, 1, fd->cidFontRevision);
	if (fd->cidCount) cffdict_input(dict, op_CIDCount, cff_INTEGER, 1, fd->cidCount);
	if (fd->UIDBase) cffdict_input(dict, op_UIDBase, cff_INTEGER, 1, fd->UIDBase);
	return dict;
}

static cff_Dict *cff_make_private_dict(cff_PrivateDict *pd) {
	cff_Dict *dict;
	NEW(dict);
	if (!pd) return dict;
	// DELTA arrays
	cffdict_input_array(dict, op_BlueValues, cff_DOUBLE, pd->blueValuesCount, pd->blueValues);
	cffdict_input_array(dict, op_OtherBlues, cff_DOUBLE, pd->otherBluesCount, pd->otherBlues);
	cffdict_input_array(dict, op_FamilyBlues, cff_DOUBLE, pd->familyBluesCount, pd->familyBlues);
	cffdict_input_array(dict, op_FamilyOtherBlues, cff_DOUBLE, pd->familyOtherBluesCount,
	                    pd->familyOtherBlues);
	cffdict_input_array(dict, op_StemSnapH, cff_DOUBLE, pd->stemSnapHCount, pd->stemSnapH);
	cffdict_input_array(dict, op_StemSnapV, cff_DOUBLE, pd->stemSnapVCount, pd->stemSnapV);

	// Private scalars
	cffdict_input(dict, op_BlueScale, cff_DOUBLE, 1, pd->blueScale);
	cffdict_input(dict, op_BlueShift, cff_DOUBLE, 1, pd->blueShift);
	cffdict_input(dict, op_BlueFuzz, cff_DOUBLE, 1, pd->blueFuzz);
	cffdict_input(dict, op_StdHW, cff_DOUBLE, 1, pd->stdHW);
	cffdict_input(dict, op_StdVW, cff_DOUBLE, 1, pd->stdVW);
	cffdict_input(dict, op_ForceBold, cff_INTEGER, 1, (int)pd->forceBold);
	cffdict_input(dict, op_LanguageGroup, cff_INTEGER, 1, pd->languageGroup);
	cffdict_input(dict, op_ExpansionFactor, cff_DOUBLE, 1, pd->expansionFactor);
	cffdict_input(dict, op_initialRandomSeed, cff_DOUBLE, 1, pd->initialRandomSeed);

	// op_nominalWidthX are currently not used
	cffdict_input(dict, op_defaultWidthX, cff_DOUBLE, 1, pd->defaultWidthX);
	cffdict_input(dict, op_nominalWidthX, cff_DOUBLE, 1, pd->nominalWidthX);

	return dict;
}

static int by_sid(cff_sid_entry *a, cff_sid_entry *b) {
	return a->sid - b->sid;
}
static caryll_Buffer *callback_makestringindex(void *context, uint32_t i) {
	caryll_Buffer **blobs = context;
	return blobs[i];
}
static caryll_Buffer *cffstrings_to_indexblob(cff_sid_entry **h) {
	HASH_SORT(*h, by_sid);
	caryll_Buffer **blobs;
	uint32_t n = HASH_COUNT(*h);
	NEW(blobs, n);
	uint32_t j = 0;
	cff_sid_entry *item, *tmp;
	HASH_ITER(hh, *h, item, tmp) {
		blobs[j] = bufnew();
		bufwrite_sds(blobs[j], item->str);
		HASH_DEL(*h, item);
		sdsfree(item->str);
		FREE(item);
		j++;
	}

	cff_Index *strings = cff_iIndex.fromCallback(blobs, n, callback_makestringindex);
	FREE(blobs);
	caryll_Buffer *final_blob = cff_iIndex.build(strings);
	cff_iIndex.free(strings);
	final_blob->cursor = final_blob->size;
	return final_blob;
}

static caryll_Buffer *cff_compile_nameindex(table_CFF *cff) {
	cff_Index *nameIndex = cff_iIndex.create();
	nameIndex->count = 1;
	nameIndex->offSize = 4;
	NEW(nameIndex->offset, 2);
	if (!cff->fontName) { cff->fontName = sdsnew("Caryll-CFF-FONT"); }
	nameIndex->offset[0] = 1;
	nameIndex->offset[1] = (uint32_t)(sdslen(cff->fontName) + 1);
	NEW(nameIndex->data, 1 + sdslen(cff->fontName));
	memcpy(nameIndex->data, cff->fontName, sdslen(cff->fontName));
	// CFF Name INDEX
	caryll_Buffer *buf = cff_iIndex.build(nameIndex);
	cff_iIndex.free(nameIndex);
	if (cff->fontName) {
		sdsfree(cff->fontName);
		cff->fontName = NULL;
	}
	return buf;
}

static caryll_Buffer *cff_make_charset(table_CFF *cff, table_glyf *glyf,
                                       cff_sid_entry **stringHash) {
	cff_Charset *charset;
	NEW(charset);
	if (glyf->length > 1) { // At least two glyphs
		charset->t = cff_CHARSET_FORMAT2;
		charset->s = 1; // One segment only
		charset->f2.format = 2;
		NEW(charset->f2.range2);
		if (cff->isCID) {
			charset->f2.range2[0].first = 1;
			charset->f2.range2[0].nleft = glyf->length - 2;
		} else {
			for (glyphid_t j = 1; j < glyf->length; j++) {
				sidof(stringHash, glyf->items[j]->name);
			}
			charset->f2.range2[0].first = sidof(stringHash, glyf->items[1]->name);
			charset->f2.range2[0].nleft = glyf->length - 2;
		}
	} else {
		charset->t = cff_CHARSET_ISOADOBE;
	}
	caryll_Buffer *c = cff_build_Charset(*charset);
	if (charset->t == cff_CHARSET_FORMAT2) { FREE(charset->f2.range2); }
	FREE(charset);
	return c;
}

static caryll_Buffer *cff_make_fdselect(table_CFF *cff, table_glyf *glyf) {
	if (!cff->isCID) return bufnew();
	// We choose format 3 here
	uint32_t ranges = 1;
	uint8_t current = 0;
	cff_FDSelect *fds;
	NEW(fds);
	fds->t = cff_FDSELECT_UNSPECED;
	if (!glyf->length) goto done;
	uint8_t fdi0 = glyf->items[0]->fdSelect.index;
	if (fdi0 > cff->fdArrayCount) fdi0 = 0;
	current = fdi0;
	for (glyphid_t j = 1; j < glyf->length; j++) {
		uint8_t fdi = glyf->items[j]->fdSelect.index;
		if (fdi > cff->fdArrayCount) fdi = 0;
		if (fdi != current) {
			current = fdi;
			ranges++;
		}
	}
	NEW(fds->f3.range3, ranges);
	fds->f3.range3[0].first = 0;
	fds->f3.range3[0].fd = current = fdi0;
	for (glyphid_t j = 1; j < glyf->length; j++) {
		uint8_t fdi = glyf->items[j]->fdSelect.index;
		if (fdi > cff->fdArrayCount) fdi = 0;
		if (glyf->items[j]->fdSelect.index != current) {
			current = fdi;
			fds->s++;
			fds->f3.range3[fds->s].first = j;
			fds->f3.range3[fds->s].fd = current;
		}
	}
	fds->t = cff_FDSELECT_FORMAT3;
	fds->s = ranges;
	fds->f3.format = 3;
	fds->f3.nranges = ranges;
	fds->f3.sentinel = glyf->length;
done:;
	caryll_Buffer *e = cff_build_FDSelect(*fds);
	cff_close_FDSelect(*fds);
	FREE(fds);
	return e;
}

typedef struct {
	table_CFF **fdArray;
	cff_sid_entry **stringHash;
} fdarray_compile_context;
static caryll_Buffer *callback_makefd(void *_context, uint32_t i) {
	fdarray_compile_context *context = (fdarray_compile_context *)_context;
	cff_Dict *fd = cff_make_fd_dict(context->fdArray[i], context->stringHash);
	caryll_Buffer *blob = cff_iDict.build(fd);
	bufwrite_bufdel(blob, cff_buildOffset(0xEEEEEEEE));
	bufwrite_bufdel(blob, cff_buildOffset(0xFFFFFFFF));
	bufwrite_bufdel(blob, cff_encodeCffOperator(op_Private));
	cff_iDict.build(fd);
	return blob;
}
static cff_Index *cff_make_fdarray(tableid_t fdArrayCount, table_CFF **fdArray,
                                   cff_sid_entry **stringHash) {
	fdarray_compile_context context;
	context.fdArray = fdArray;
	context.stringHash = stringHash;

	return cff_iIndex.fromCallback(&context, fdArrayCount, callback_makefd);
}

static caryll_Buffer *writecff_CIDKeyed(table_CFF *cff, table_glyf *glyf,
                                        const otfcc_Options *options) {
	caryll_Buffer *blob = bufnew();
	// The Strings hashtable
	cff_sid_entry *stringHash = NULL;

	// CFF Header
	caryll_Buffer *h = cff_buildHeader();
	caryll_Buffer *n = cff_compile_nameindex(cff);

	// cff top DICT
	cff_Dict *top = cff_make_fd_dict(cff, &stringHash);
	caryll_Buffer *t = cff_iDict.build(top);
	cff_iDict.free(top);

	// cff top PRIVATE
	cff_Dict *top_pd = cff_make_private_dict(cff->privateDict);
	caryll_Buffer *p = cff_iDict.build(top_pd);
	bufwrite_bufdel(p, cff_buildOffset(0xFFFFFFFF));
	bufwrite_bufdel(p, cff_encodeCffOperator(op_Subrs));
	cff_iDict.free(top_pd);

	// FDSelect
	caryll_Buffer *e = cff_make_fdselect(cff, glyf);

	// FDArray
	cff_Index *fdArrayIndex = NULL;
	caryll_Buffer *r;
	if (cff->isCID) {
		fdArrayIndex = cff_make_fdarray(cff->fdArrayCount, cff->fdArray, &stringHash);
		r = cff_iIndex.build(fdArrayIndex);
	} else {
		NEW(r);
	}

	// cff charset : Allocate string index terms for glyph names
	caryll_Buffer *c = cff_make_charset(cff, glyf, &stringHash);

	// CFF String Index
	caryll_Buffer *i = cffstrings_to_indexblob(&stringHash);

	// CFF Charstrings
	caryll_Buffer *s;
	caryll_Buffer *gs;
	caryll_Buffer *ls;
	{
		cff_charstring_builder_context g2cContext;
		g2cContext.glyf = glyf;
		g2cContext.defaultWidth = cff->privateDict->defaultWidthX;
		g2cContext.nominalWidthX = cff->privateDict->nominalWidthX;
		g2cContext.options = options;
		cff_iSubrGraph.init(&g2cContext.graph);
		g2cContext.graph.doSubroutinize = options->cff_doSubroutinize;

		cff_make_charstrings(&g2cContext, &s, &gs, &ls);

		cff_iSubrGraph.dispose(&g2cContext.graph);
	}

	// Merge these data
	uint32_t additionalTopDictOpsSize = 0;
	uint32_t off = (uint32_t)(h->size + n->size + 11 + t->size);
	if (c->size != 0) additionalTopDictOpsSize += 6;  // op_charset
	if (e->size != 0) additionalTopDictOpsSize += 7;  // op_FDSelect
	if (s->size != 0) additionalTopDictOpsSize += 6;  // op_CharStrings
	if (p->size != 0) additionalTopDictOpsSize += 11; // op_Private
	if (r->size != 0) additionalTopDictOpsSize += 7;  // op_FDArray

	// Start building CFF table
	bufwrite_bufdel(blob, h); // header
	bufwrite_bufdel(blob, n); // name index
	int32_t delta_size = (uint32_t)(t->size + additionalTopDictOpsSize + 1);
	bufwrite_bufdel(blob, bufninit(11, 0, 1,   // top dict index headerone item
	                               4,          // four bytes per offset
	                               0, 0, 0, 1, // offset 1
	                               (delta_size >> 24) & 0xff, (delta_size >> 16) & 0xff,
	                               (delta_size >> 8) & 0xff,
	                               delta_size & 0xff) // offset 2
	);

	bufwrite_bufdel(blob, t); // top dict body

	// top dict offsets
	off += additionalTopDictOpsSize + i->size + gs->size;
	if (c->size != 0) {
		bufwrite_bufdel(blob, cff_buildOffset(off));
		bufwrite_bufdel(blob, cff_encodeCffOperator(op_charset));
		off += c->size;
	}
	if (e->size != 0) {
		bufwrite_bufdel(blob, cff_buildOffset(off));
		bufwrite_bufdel(blob, cff_encodeCffOperator(op_FDSelect));
		off += e->size;
	}
	if (s->size != 0) {
		bufwrite_bufdel(blob, cff_buildOffset(off));
		bufwrite_bufdel(blob, cff_encodeCffOperator(op_CharStrings));
		off += s->size;
	}
	if (p->size != 0) {
		bufwrite_bufdel(blob, cff_buildOffset((uint32_t)(p->size)));
		bufwrite_bufdel(blob, cff_buildOffset(off));
		bufwrite_bufdel(blob, cff_encodeCffOperator(op_Private));
		off += p->size;
	}
	if (r->size != 0) {
		bufwrite_bufdel(blob, cff_buildOffset(off));
		bufwrite_bufdel(blob, cff_encodeCffOperator(op_FDArray));
		off += r->size;
	}

	bufwrite_bufdel(blob, i);  // string index
	bufwrite_bufdel(blob, gs); // gsubr
	bufwrite_bufdel(blob, c);  // charset
	bufwrite_bufdel(blob, e);  // fdselect
	bufwrite_bufdel(blob, s);  // charstring
	size_t *startingPositionOfPrivates;
	NEW(startingPositionOfPrivates, 1 + cff->fdArrayCount);
	startingPositionOfPrivates[0] = blob->cursor;
	bufwrite_bufdel(blob, p); // top private
	size_t *endingPositionOfPrivates;
	NEW(endingPositionOfPrivates, 1 + cff->fdArrayCount);
	endingPositionOfPrivates[0] = blob->cursor;
	if (cff->isCID) { // fdarray and fdarray privates
		uint32_t fdArrayPrivatesStartOffset = off;
		caryll_Buffer **fdArrayPrivates;
		NEW(fdArrayPrivates, cff->fdArrayCount);
		for (tableid_t j = 0; j < cff->fdArrayCount; j++) {
			cff_Dict *pd = cff_make_private_dict(cff->fdArray[j]->privateDict);
			caryll_Buffer *p = cff_iDict.build(pd);
			bufwrite_bufdel(p, cff_buildOffset(0xFFFFFFFF));
			bufwrite_bufdel(p, cff_encodeCffOperator(op_Subrs));
			cff_iDict.free(pd);
			fdArrayPrivates[j] = p;
			uint8_t *privateLengthPtr = &(fdArrayIndex->data[fdArrayIndex->offset[j + 1] - 11]);
			privateLengthPtr[0] = (p->size >> 24) & 0xFF;
			privateLengthPtr[1] = (p->size >> 16) & 0xFF;
			privateLengthPtr[2] = (p->size >> 8) & 0xFF;
			privateLengthPtr[3] = (p->size >> 0) & 0xFF;
			uint8_t *privateOffsetPtr = &(fdArrayIndex->data[fdArrayIndex->offset[j + 1] - 6]);
			privateOffsetPtr[0] = (fdArrayPrivatesStartOffset >> 24) & 0xFF;
			privateOffsetPtr[1] = (fdArrayPrivatesStartOffset >> 16) & 0xFF;
			privateOffsetPtr[2] = (fdArrayPrivatesStartOffset >> 8) & 0xFF;
			privateOffsetPtr[3] = (fdArrayPrivatesStartOffset >> 0) & 0xFF;
			fdArrayPrivatesStartOffset += p->size;
		}
		buffree(r); // fdarray
		r = cff_iIndex.build(fdArrayIndex);
		cff_iIndex.free(fdArrayIndex);
		bufwrite_bufdel(blob, r);
		for (tableid_t j = 0; j < cff->fdArrayCount; j++) {
			startingPositionOfPrivates[j + 1] = blob->cursor;
			bufwrite_bufdel(blob, fdArrayPrivates[j]);
			endingPositionOfPrivates[j + 1] = blob->cursor;
		}
		FREE(fdArrayPrivates);
	} else {
		bufwrite_bufdel(blob, r);
	}
	size_t positionOfLocalSubroutines = blob->cursor;
	bufwrite_bufdel(blob, ls);
	for (tableid_t j = 0; j < cff->fdArrayCount + 1; j++) {
		size_t lsOffset = positionOfLocalSubroutines - startingPositionOfPrivates[j];
		uint8_t *ptr = &(blob->data[endingPositionOfPrivates[j] - 5]);
		ptr[0] = (lsOffset >> 24) & 0xFF;
		ptr[1] = (lsOffset >> 16) & 0xFF;
		ptr[2] = (lsOffset >> 8) & 0xFF;
		ptr[3] = (lsOffset >> 0) & 0xFF;
	}
	FREE(startingPositionOfPrivates);
	FREE(endingPositionOfPrivates);
	// finish
	return blob;
}

caryll_Buffer *otfcc_buildCFF(const table_CFFAndGlyf cffAndGlyf, const otfcc_Options *options) {
	return writecff_CIDKeyed(cffAndGlyf.meta, cffAndGlyf.glyphs, options);
}
