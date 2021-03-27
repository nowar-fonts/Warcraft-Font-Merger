#include "../glyf.h"

#include "support/util.h"
#include "support/ttinstr/ttinstr.h"

caryll_Buffer *shrinkFlags(caryll_Buffer *flags) {
	if (!buflen(flags)) return (flags);
	caryll_Buffer *shrunk = bufnew();
	bufwrite8(shrunk, flags->data[0]);
	int repeating = 0;
	for (size_t j = 1; j < buflen(flags); j++) {
		if (flags->data[j] == flags->data[j - 1]) {
			if (repeating && repeating < 0xFE) {
				shrunk->data[shrunk->cursor - 1] += 1;
				repeating += 1;
			} else if (repeating == 0) {
				shrunk->data[shrunk->cursor - 1] |= GLYF_FLAG_REPEAT;
				bufwrite8(shrunk, 1);
				repeating += 1;
			} else {
				repeating = 0;
				bufwrite8(shrunk, flags->data[j]);
			}
		} else {
			repeating = 0;
			bufwrite8(shrunk, flags->data[j]);
		}
	}
	buffree(flags);
	return shrunk;
}

// serialize
#define EPSILON (1e-5)
static void glyf_build_simple(const glyf_Glyph *g, caryll_Buffer *gbuf) {
	caryll_Buffer *flags = bufnew();
	caryll_Buffer *xs = bufnew();
	caryll_Buffer *ys = bufnew();

	bufwrite16b(gbuf, g->contours.length);
	bufwrite16b(gbuf, (int16_t)g->stat.xMin);
	bufwrite16b(gbuf, (int16_t)g->stat.yMin);
	bufwrite16b(gbuf, (int16_t)g->stat.xMax);
	bufwrite16b(gbuf, (int16_t)g->stat.yMax);

	// endPtsOfContours[n]
	shapeid_t ptid = 0;
	for (shapeid_t j = 0; j < g->contours.length; j++) {
		ptid += g->contours.items[j].length;
		bufwrite16b(gbuf, ptid - 1);
	}

	// instructions
	bufwrite16b(gbuf, g->instructionsLength);
	if (g->instructions) bufwrite_bytes(gbuf, g->instructionsLength, g->instructions);

	// flags and points
	bufclear(flags);
	bufclear(xs);
	bufclear(ys);
	int32_t cx = 0;
	int32_t cy = 0;
	for (shapeid_t cj = 0; cj < g->contours.length; cj++) {
		for (shapeid_t k = 0; k < g->contours.items[cj].length; k++) {
			glyf_Point *p = &(g->contours.items[cj].items[k]);
			uint8_t flag = (p->onCurve & MASK_ON_CURVE) ? GLYF_FLAG_ON_CURVE : 0;
			int32_t px = round(iVQ.getStill(p->x));
			int32_t py = round(iVQ.getStill(p->y));
			int16_t dx = (int16_t)(px - cx);
			int16_t dy = (int16_t)(py - cy);
			if (dx == 0) {
				flag |= GLYF_FLAG_SAME_X;
			} else if (dx >= -0xFF && dx <= 0xFF) {
				flag |= GLYF_FLAG_X_SHORT;
				if (dx > 0) {
					flag |= GLYF_FLAG_POSITIVE_X;
					bufwrite8(xs, dx);
				} else {
					bufwrite8(xs, -dx);
				}
			} else {
				bufwrite16b(xs, dx);
			}

			if (dy == 0) {
				flag |= GLYF_FLAG_SAME_Y;
			} else if (dy >= -0xFF && dy <= 0xFF) {
				flag |= GLYF_FLAG_Y_SHORT;
				if (dy > 0) {
					flag |= GLYF_FLAG_POSITIVE_Y;
					bufwrite8(ys, dy);
				} else {
					bufwrite8(ys, -dy);
				}
			} else {
				bufwrite16b(ys, dy);
			}
			bufwrite8(flags, flag);
			cx = px;
			cy = py;
		}
	}
	flags = shrinkFlags(flags);
	bufwrite_buf(gbuf, flags);
	bufwrite_buf(gbuf, xs);
	bufwrite_buf(gbuf, ys);

	buffree(flags);
	buffree(xs);
	buffree(ys);
}
static void glyf_build_composite(const glyf_Glyph *g, caryll_Buffer *gbuf) {
	bufwrite16b(gbuf, (-1));
	bufwrite16b(gbuf, (int16_t)g->stat.xMin);
	bufwrite16b(gbuf, (int16_t)g->stat.yMin);
	bufwrite16b(gbuf, (int16_t)g->stat.xMax);
	bufwrite16b(gbuf, (int16_t)g->stat.yMax);
	for (shapeid_t rj = 0; rj < g->references.length; rj++) {
		glyf_ComponentReference *r = &(g->references.items[rj]);
		uint16_t flags =
		    (rj < g->references.length - 1 ? MORE_COMPONENTS
		                                   : g->instructionsLength > 0 ? WE_HAVE_INSTRUCTIONS : 0);
		bool outputAnchor = r->isAnchored == REF_ANCHOR_CONSOLIDATED;

		union {
			uint16_t pointid;
			int16_t coord;
		} arg1, arg2;

		// flags
		if (outputAnchor) {
			arg1.pointid = r->outer;
			arg2.pointid = r->inner;
			if (!(arg1.pointid < 0x100 && arg2.pointid < 0x100)) { flags |= ARG_1_AND_2_ARE_WORDS; }
		} else {
			flags |= ARGS_ARE_XY_VALUES;
			arg1.coord = iVQ.getStill(r->x);
			arg2.coord = iVQ.getStill(r->y);
			if (!(arg1.coord < 128 && arg1.coord >= -128 && arg2.coord < 128 &&
			      arg2.coord >= -128)) {
				flags |= ARG_1_AND_2_ARE_WORDS;
			}
		}
		if (fabs(r->b) > EPSILON || fabs(r->c) > EPSILON) {
			flags |= WE_HAVE_A_TWO_BY_TWO;
		} else if (fabs(r->a - 1) > EPSILON || fabs(r->d - 1) > EPSILON) {
			if (fabs(r->a - r->d) > EPSILON) {
				flags |= WE_HAVE_AN_X_AND_Y_SCALE;
			} else {
				flags |= WE_HAVE_A_SCALE;
			}
		}
		if (r->roundToGrid) flags |= ROUND_XY_TO_GRID;
		if (r->useMyMetrics) flags |= USE_MY_METRICS;
		flags |= UNSCALED_COMPONENT_OFFSET;
		bufwrite16b(gbuf, flags);
		bufwrite16b(gbuf, r->glyph.index);
		if (flags & ARG_1_AND_2_ARE_WORDS) {
			bufwrite16b(gbuf, arg1.pointid);
			bufwrite16b(gbuf, arg2.pointid);
		} else {
			bufwrite8(gbuf, arg1.pointid);
			bufwrite8(gbuf, arg2.pointid);
		}
		if (flags & WE_HAVE_A_SCALE) {
			bufwrite16b(gbuf, otfcc_to_f2dot14(r->a));
		} else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) {
			bufwrite16b(gbuf, otfcc_to_f2dot14(r->a));
			bufwrite16b(gbuf, otfcc_to_f2dot14(r->d));
		} else if (flags & WE_HAVE_A_TWO_BY_TWO) {
			bufwrite16b(gbuf, otfcc_to_f2dot14(r->a));
			bufwrite16b(gbuf, otfcc_to_f2dot14(r->b));
			bufwrite16b(gbuf, otfcc_to_f2dot14(r->c));
			bufwrite16b(gbuf, otfcc_to_f2dot14(r->d));
		}
	}
	if (g->instructionsLength) {
		bufwrite16b(gbuf, g->instructionsLength);
		if (g->instructions) bufwrite_bytes(gbuf, g->instructionsLength, g->instructions);
	}
}
table_GlyfAndLocaBuffers otfcc_buildGlyf(const table_glyf *table, table_head *head,
                                         const otfcc_Options *options) {
	caryll_Buffer *bufglyf = bufnew();
	caryll_Buffer *bufloca = bufnew();
	if (table && head) {
		caryll_Buffer *gbuf = bufnew();
		uint32_t *loca;
		NEW(loca, table->length + 1);
		for (glyphid_t j = 0; j < table->length; j++) {
			loca[j] = (uint32_t)bufglyf->cursor;
			glyf_Glyph *g = table->items[j];
			bufclear(gbuf);
			if (g->contours.length > 0) {
				glyf_build_simple(g, gbuf);
			} else if (g->references.length > 0) {
				glyf_build_composite(g, gbuf);
			}
			// pad extra zeroes
			buflongalign(gbuf);
			bufwrite_buf(bufglyf, gbuf);
		}
		loca[table->length] = (uint32_t)bufglyf->cursor;
		if (bufglyf->cursor >= 0x20000) {
			head->indexToLocFormat = 1;
		} else {
			head->indexToLocFormat = 0;
		}
		// write loca table
		for (uint32_t j = 0; j <= table->length; j++) {
			if (head->indexToLocFormat) {
				bufwrite32b(bufloca, loca[j]);
			} else {
				bufwrite16b(bufloca, loca[j] >> 1);
			}
		}
		buffree(gbuf);
		FREE(loca);
	}
	table_GlyfAndLocaBuffers pair = {bufglyf, bufloca};
	return pair;
}
