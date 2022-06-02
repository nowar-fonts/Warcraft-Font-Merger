#include "stat.h"

#include <time.h>
#include <float.h>
#include "support/util.h"

// Stating
// Calculate necessary values for SFNT

typedef enum { stat_not_started = 0, stat_doing = 1, stat_completed = 2 } stat_status;

glyf_GlyphStat stat_single_glyph(table_glyf *table, glyf_ComponentReference *gr,
                                 stat_status *stated, uint8_t depth, glyphid_t topj,
                                 const otfcc_Options *options) {
	glyf_GlyphStat stat = {0, 0, 0, 0, 0, 0, 0, 0, 0};
	glyphid_t j = gr->glyph.index;
	if (depth >= 0xFF) return stat;
	if (stated[j] == stat_doing) {
		// We have a circular reference
		logWarning("[Stat] Circular glyph reference found in gid %d to gid %d. The reference will "
		           "be dropped.\n",
		           topj, j);
		stated[j] = stat_completed;
		return stat;
	}

	glyf_Glyph *g = table->items[j];
	stated[j] = stat_doing;
	pos_t xmin = POS_MAX;
	pos_t xmax = -POS_MAX;
	pos_t ymin = POS_MAX;
	pos_t ymax = -POS_MAX;
	uint16_t nestDepth = 0;
	uint16_t nPoints = 0;
	uint16_t nCompositePoints = 0;
	uint16_t nCompositeContours = 0;
	// Stat xmin, xmax, ymin, ymax
	for (shapeid_t c = 0; c < g->contours.length; c++) {
		for (shapeid_t pj = 0; pj < g->contours.items[c].length; pj++) {
			// Stat point coordinates USING the matrix transformation
			glyf_Point *p = &(g->contours.items[c].items[pj]);
			pos_t x = round(iVQ.getStill(gr->x) + gr->a * iVQ.getStill(p->x) +
			                gr->b * iVQ.getStill(p->y));
			pos_t y = round(iVQ.getStill(gr->y) + gr->c * iVQ.getStill(p->x) +
			                gr->d * iVQ.getStill(p->y));
			if (x < xmin) xmin = x;
			if (x > xmax) xmax = x;
			if (y < ymin) ymin = y;
			if (y > ymax) ymax = y;
			nPoints += 1;
		}
	}
	nCompositePoints = nPoints;
	nCompositeContours = g->contours.length;
	for (shapeid_t r = 0; r < g->references.length; r++) {
		glyf_ComponentReference ref;
		glyf_iComponentReference.init(&ref);
		glyf_ComponentReference *rr = &(g->references.items[r]);
		// composite affine transformations
		Handle.replace(&ref.glyph, Handle.fromIndex(g->references.items[r].glyph.index));
		ref.a = gr->a * rr->a + rr->b * gr->c;
		ref.b = rr->a * gr->b + rr->b * gr->d;
		ref.c = gr->a * rr->c + gr->c * rr->d;
		ref.d = gr->b * rr->c + rr->d * gr->d;
		iVQ.replace(&ref.x, iVQ.createStill(iVQ.getStill(rr->x) + rr->a * iVQ.getStill(gr->x) +
		                                    rr->b * iVQ.getStill(gr->y)));
		iVQ.replace(&ref.y, iVQ.createStill(iVQ.getStill(rr->y) + rr->c * iVQ.getStill(gr->x) +
		                                    rr->d * iVQ.getStill(gr->y)));

		glyf_GlyphStat thatstat = stat_single_glyph(table, &ref, stated, depth + 1, topj, options);
		if (thatstat.xMin < xmin) xmin = thatstat.xMin;
		if (thatstat.xMax > xmax) xmax = thatstat.xMax;
		if (thatstat.yMin < ymin) ymin = thatstat.yMin;
		if (thatstat.yMax > ymax) ymax = thatstat.yMax;
		if (thatstat.nestDepth + 1 > nestDepth) nestDepth = thatstat.nestDepth + 1;
		nCompositePoints += thatstat.nCompositePoints;
		nCompositeContours += thatstat.nCompositeContours;
	}
	// deal with space glyphs
	if (xmin > xmax) xmin = xmax = 0;
	if (ymin > ymax) ymin = ymax = 0;

	// write back
	stat.xMin = xmin;
	stat.xMax = xmax;
	stat.yMin = ymin;
	stat.yMax = ymax;
	stat.nestDepth = nestDepth;
	stat.nPoints = nPoints;
	stat.nContours = g->contours.length;
	stat.nCompositePoints = nCompositePoints;
	stat.nCompositeContours = nCompositeContours;
	stated[j] = stat_completed;
	return stat;
}

void statGlyf(otfcc_Font *font, const otfcc_Options *options) {
	stat_status *stated;
	NEW(stated, font->glyf->length);
	pos_t xmin = 0xFFFFFFFF;
	pos_t xmax = -0xFFFFFFFF;
	pos_t ymin = 0xFFFFFFFF;
	pos_t ymax = -0xFFFFFFFF;
	for (glyphid_t j = 0; j < font->glyf->length; j++) {
		glyf_ComponentReference gr;
		gr.glyph = Handle.fromIndex(j);
		gr.x = iVQ.createStill(0);
		gr.y = iVQ.createStill(0);
		gr.a = 1;
		gr.b = 0;
		gr.c = 0;
		gr.d = 1;
		glyf_GlyphStat thatstat = font->glyf->items[j]->stat =
		    stat_single_glyph(font->glyf, &gr, stated, 0, j, options);
		if (thatstat.xMin < xmin) xmin = thatstat.xMin;
		if (thatstat.xMax > xmax) xmax = thatstat.xMax;
		if (thatstat.yMin < ymin) ymin = thatstat.yMin;
		if (thatstat.yMax > ymax) ymax = thatstat.yMax;
	}
	font->head->xMin = xmin;
	font->head->xMax = xmax;
	font->head->yMin = ymin;
	font->head->yMax = ymax;
	FREE(stated);
}

void statMaxp(otfcc_Font *font) {
	uint16_t nestDepth = 0;
	uint16_t nPoints = 0;
	uint16_t nContours = 0;
	uint16_t nComponents = 0;
	uint16_t nCompositePoints = 0;
	uint16_t nCompositeContours = 0;
	uint16_t instSize = 0;
	for (glyphid_t j = 0; j < font->glyf->length; j++) {
		glyf_Glyph *g = font->glyf->items[j];
		if (g->contours.length > 0) {
			if (g->stat.nPoints > nPoints) nPoints = g->stat.nPoints;
			if (g->stat.nContours > nContours) nContours = g->stat.nContours;
		} else if (g->references.length > 0) {
			if (g->stat.nCompositePoints > nCompositePoints)
				nCompositePoints = g->stat.nCompositePoints;
			if (g->stat.nCompositeContours > nCompositeContours)
				nCompositeContours = g->stat.nCompositeContours;
			if (g->stat.nestDepth > nestDepth) nestDepth = g->stat.nestDepth;
			if (g->references.length > nComponents) nComponents = g->references.length;
		}
		if (g->instructionsLength > instSize) instSize = g->instructionsLength;
	}
	font->maxp->maxPoints = nPoints;
	font->maxp->maxContours = nContours;
	font->maxp->maxCompositePoints = nCompositePoints;
	font->maxp->maxCompositeContours = nCompositeContours;
	font->maxp->maxComponentDepth = nestDepth;
	font->maxp->maxComponentElements = nComponents;
	font->maxp->maxSizeOfInstructions = instSize;
}

static void statHmtx(otfcc_Font *font, const otfcc_Options *options) {
	if (!font->glyf) return;
	table_hmtx *hmtx;
	NEW(hmtx);
	glyphid_t count_a = font->glyf->length;
	glyphid_t count_k = 0;
	bool lsbAtX_0 = true;
	if (font->subtype == FONTTYPE_CFF) {
		// pass
	} else {
		while (count_a > 2 && iVQ.getStill(font->glyf->items[count_a - 1]->advanceWidth) ==
		                          iVQ.getStill(font->glyf->items[count_a - 2]->advanceWidth)) {
			count_a--;
		}
		count_k = font->glyf->length - count_a;
	}
	NEW(hmtx->metrics, count_a);
	NEW(hmtx->leftSideBearing, count_k);

	pos_t minLSB = 0x7FFF;
	pos_t minRSB = 0x7FFF;
	pos_t maxExtent = -0x8000;
	length_t maxWidth = 0;
	for (glyphid_t j = 0; j < font->glyf->length; j++) {
		glyf_Glyph *g = font->glyf->items[j];
		if (iVQ.isZero(g->horizontalOrigin, 1.0 / 1000.0)) {
			iVQ.replace(&g->horizontalOrigin, iVQ.neutral());
		} else {
			lsbAtX_0 = false;
		}
		const pos_t hori = iVQ.getStill(g->horizontalOrigin);
		const pos_t advw = iVQ.getStill(g->advanceWidth);
		const pos_t lsb = (g->stat.xMin) - hori;
		const pos_t rsb = advw + hori - (g->stat.xMax);

		if (j < count_a) {
			hmtx->metrics[j].advanceWidth = advw;
			hmtx->metrics[j].lsb = lsb;
		} else {
			hmtx->leftSideBearing[j - count_a] = lsb;
		}

		if (advw > maxWidth) maxWidth = advw;
		if (lsb < minLSB) minLSB = lsb;
		if (rsb < minRSB) minRSB = rsb;
		if (g->stat.xMax - hori > maxExtent) { maxExtent = g->stat.xMax - hori; }
	}
	font->hhea->numberOfMetrics = count_a;
	font->hhea->minLeftSideBearing = minLSB;
	font->hhea->minRightSideBearing = minRSB;
	font->hhea->xMaxExtent = maxExtent;
	font->hhea->advanceWidthMax = maxWidth;
	font->hmtx = hmtx;
	// set bit 1 in head.flags
	font->head->flags = (font->head->flags & (~0x2)) | (lsbAtX_0 ? 0x2 : 0);
}
static void statVmtx(otfcc_Font *font, const otfcc_Options *options) {
	if (!font->glyf) return;
	table_vmtx *vmtx;
	NEW(vmtx);
	glyphid_t count_a = font->glyf->length;
	glyphid_t count_k = 0;
	if (font->subtype == FONTTYPE_CFF && !options->cff_short_vmtx) {
		// pass
	} else {
		while (count_a > 2 && iVQ.getStill(font->glyf->items[count_a - 1]->advanceHeight) ==
		                          iVQ.getStill(font->glyf->items[count_a - 2]->advanceHeight)) {
			count_a--;
		}
		count_k = font->glyf->length - count_a;
	}
	NEW(vmtx->metrics, count_a);
	NEW(vmtx->topSideBearing, count_k);

	pos_t minTSB = 0x7FFF;
	pos_t minBSB = 0x7FFF;
	pos_t maxExtent = -0x8000;
	length_t maxHeight = 0;
	for (glyphid_t j = 0; j < font->glyf->length; j++) {
		glyf_Glyph *g = font->glyf->items[j];
		const pos_t vori = iVQ.getStill(g->verticalOrigin);
		const pos_t advh = iVQ.getStill(g->advanceHeight);
		const pos_t tsb = (vori) - (g->stat.yMax);
		const pos_t bsb = (g->stat.yMin) - (vori) + (advh);
		if (j < count_a) {
			vmtx->metrics[j].advanceHeight = advh;
			vmtx->metrics[j].tsb = tsb;
		} else {
			vmtx->topSideBearing[j - count_a] = tsb;
		}
		if (advh > maxHeight) maxHeight = advh;
		if (tsb < minTSB) minTSB = tsb;
		if (bsb < minBSB) minBSB = bsb;
		if (vori - g->stat.yMin > maxExtent) { maxExtent = vori - g->stat.yMin; }
	}
	font->vhea->numOfLongVerMetrics = count_a;
	font->vhea->minTop = minTSB;
	font->vhea->minBottom = minBSB;
	font->vhea->yMaxExtent = maxExtent;
	font->vhea->advanceHeightMax = maxHeight;
	font->vmtx = vmtx;
}
static void statOS_2UnicodeRanges(otfcc_Font *font, const otfcc_Options *options) {
	cmap_Entry *item;
	// Stat for OS/2.ulUnicodeRange.
	uint32_t u1 = 0;
	uint32_t u2 = 0;
	uint32_t u3 = 0;
	uint32_t u4 = 0;
	int32_t minUnicode = 0xFFFF;
	int32_t maxUnicode = 0;
	foreach_hash(item, font->cmap->unicodes) {
		int u = item->unicode;
		// Stat for minimium and maximium unicode
		if (u < minUnicode) minUnicode = u;
		if (u > maxUnicode) maxUnicode = u;
		// Reference: https://www.microsoft.com/typography/otspec/os2.htm#ur
		if ((u >= 0x0000 && u <= 0x007F)) { u1 |= (1 << 0); }
		if ((u >= 0x0080 && u <= 0x00FF)) { u1 |= (1 << 1); }
		if ((u >= 0x0100 && u <= 0x017F)) { u1 |= (1 << 2); }
		if ((u >= 0x0180 && u <= 0x024F)) { u1 |= (1 << 3); }
		if ((u >= 0x0250 && u <= 0x02AF) || (u >= 0x1D00 && u <= 0x1D7F) ||
		    (u >= 0x1D80 && u <= 0x1DBF)) {
			u1 |= (1 << 4);
		}
		if ((u >= 0x02B0 && u <= 0x02FF) || (u >= 0xA700 && u <= 0xA71F)) { u1 |= (1 << 5); }
		if ((u >= 0x0300 && u <= 0x036F) || (u >= 0x1DC0 && u <= 0x1DFF)) { u1 |= (1 << 6); }
		if ((u >= 0x0370 && u <= 0x03FF)) { u1 |= (1 << 7); }
		if ((u >= 0x2C80 && u <= 0x2CFF)) { u1 |= (1 << 8); }
		if ((u >= 0x0400 && u <= 0x04FF) || (u >= 0x0500 && u <= 0x052F) ||
		    (u >= 0x2DE0 && u <= 0x2DFF) || (u >= 0xA640 && u <= 0xA69F)) {
			u1 |= (1 << 9);
		}
		if ((u >= 0x0530 && u <= 0x058F)) { u1 |= (1 << 10); }
		if ((u >= 0x0590 && u <= 0x05FF)) { u1 |= (1 << 11); }
		if ((u >= 0xA500 && u <= 0xA63F)) { u1 |= (1 << 12); }
		if ((u >= 0x0600 && u <= 0x06FF) || (u >= 0x0750 && u <= 0x077F)) { u1 |= (1 << 13); }
		if ((u >= 0x07C0 && u <= 0x07FF)) { u1 |= (1 << 14); }
		if ((u >= 0x0900 && u <= 0x097F)) { u1 |= (1 << 15); }
		if ((u >= 0x0980 && u <= 0x09FF)) { u1 |= (1 << 16); }
		if ((u >= 0x0A00 && u <= 0x0A7F)) { u1 |= (1 << 17); }
		if ((u >= 0x0A80 && u <= 0x0AFF)) { u1 |= (1 << 18); }
		if ((u >= 0x0B00 && u <= 0x0B7F)) { u1 |= (1 << 19); }
		if ((u >= 0x0B80 && u <= 0x0BFF)) { u1 |= (1 << 20); }
		if ((u >= 0x0C00 && u <= 0x0C7F)) { u1 |= (1 << 21); }
		if ((u >= 0x0C80 && u <= 0x0CFF)) { u1 |= (1 << 22); }
		if ((u >= 0x0D00 && u <= 0x0D7F)) { u1 |= (1 << 23); }
		if ((u >= 0x0E00 && u <= 0x0E7F)) { u1 |= (1 << 24); }
		if ((u >= 0x0E80 && u <= 0x0EFF)) { u1 |= (1 << 25); }
		if ((u >= 0x10A0 && u <= 0x10FF) || (u >= 0x2D00 && u <= 0x2D2F)) { u1 |= (1 << 26); }
		if ((u >= 0x1B00 && u <= 0x1B7F)) { u1 |= (1 << 27); }
		if ((u >= 0x1100 && u <= 0x11FF)) { u1 |= (1 << 28); }
		if ((u >= 0x1E00 && u <= 0x1EFF) || (u >= 0x2C60 && u <= 0x2C7F) ||
		    (u >= 0xA720 && u <= 0xA7FF)) {
			u1 |= (1 << 29);
		}
		if ((u >= 0x1F00 && u <= 0x1FFF)) { u1 |= (1 << 30); }
		if ((u >= 0x2000 && u <= 0x206F) || (u >= 0x2E00 && u <= 0x2E7F)) { u1 |= (1 << 31); }
		if ((u >= 0x2070 && u <= 0x209F)) { u2 |= (1 << 0); }
		if ((u >= 0x20A0 && u <= 0x20CF)) { u2 |= (1 << 1); }
		if ((u >= 0x20D0 && u <= 0x20FF)) { u2 |= (1 << 2); }
		if ((u >= 0x2100 && u <= 0x214F)) { u2 |= (1 << 3); }
		if ((u >= 0x2150 && u <= 0x218F)) { u2 |= (1 << 4); }
		if ((u >= 0x2190 && u <= 0x21FF) || (u >= 0x27F0 && u <= 0x27FF) ||
		    (u >= 0x2900 && u <= 0x297F) || (u >= 0x2B00 && u <= 0x2BFF)) {
			u2 |= (1 << 5);
		}
		if ((u >= 0x2200 && u <= 0x22FF) || (u >= 0x2A00 && u <= 0x2AFF) ||
		    (u >= 0x27C0 && u <= 0x27EF) || (u >= 0x2980 && u <= 0x29FF)) {
			u2 |= (1 << 6);
		}
		if ((u >= 0x2300 && u <= 0x23FF)) { u2 |= (1 << 7); }
		if ((u >= 0x2400 && u <= 0x243F)) { u2 |= (1 << 8); }
		if ((u >= 0x2440 && u <= 0x245F)) { u2 |= (1 << 9); }
		if ((u >= 0x2460 && u <= 0x24FF)) { u2 |= (1 << 10); }
		if ((u >= 0x2500 && u <= 0x257F)) { u2 |= (1 << 11); }
		if ((u >= 0x2580 && u <= 0x259F)) { u2 |= (1 << 12); }
		if ((u >= 0x25A0 && u <= 0x25FF)) { u2 |= (1 << 13); }
		if ((u >= 0x2600 && u <= 0x26FF)) { u2 |= (1 << 14); }
		if ((u >= 0x2700 && u <= 0x27BF)) { u2 |= (1 << 15); }
		if ((u >= 0x3000 && u <= 0x303F)) { u2 |= (1 << 16); }
		if ((u >= 0x3040 && u <= 0x309F)) { u2 |= (1 << 17); }
		if ((u >= 0x30A0 && u <= 0x30FF) || (u >= 0x31F0 && u <= 0x31FF)) { u2 |= (1 << 18); }
		if ((u >= 0x3100 && u <= 0x312F) || (u >= 0x31A0 && u <= 0x31BF)) { u2 |= (1 << 19); }
		if ((u >= 0x3130 && u <= 0x318F)) { u2 |= (1 << 20); }
		if ((u >= 0xA840 && u <= 0xA87F)) { u2 |= (1 << 21); }
		if ((u >= 0x3200 && u <= 0x32FF)) { u2 |= (1 << 22); }
		if ((u >= 0x3300 && u <= 0x33FF)) { u2 |= (1 << 23); }
		if ((u >= 0xAC00 && u <= 0xD7AF)) { u2 |= (1 << 24); }
		if ((u >= 0xD800 && u <= 0xDFFF) || u > 0xFFFF) { u2 |= (1 << 25); }
		if ((u >= 0x10900 && u <= 0x1091F)) { u2 |= (1 << 26); }
		if ((u >= 0x4E00 && u <= 0x9FFF) || (u >= 0x2E80 && u <= 0x2EFF) ||
		    (u >= 0x2F00 && u <= 0x2FDF) || (u >= 0x2FF0 && u <= 0x2FFF) ||
		    (u >= 0x3400 && u <= 0x4DBF) || (u >= 0x20000 && u <= 0x2F7FF) ||
		    (u >= 0x3190 && u <= 0x319F)) {
			u2 |= (1 << 27);
		}
		if ((u >= 0xE000 && u <= 0xF8FF)) { u2 |= (1 << 28); }
		if ((u >= 0x31C0 && u <= 0x31EF) || (u >= 0xF900 && u <= 0xFAFF) ||
		    (u >= 0x2F800 && u <= 0x2FA1F)) {
			u2 |= (1 << 29);
		}
		if ((u >= 0xFB00 && u <= 0xFB4F)) { u2 |= (1 << 30); }
		if ((u >= 0xFB50 && u <= 0xFDFF)) { u2 |= (1 << 31); }
		if ((u >= 0xFE20 && u <= 0xFE2F)) { u3 |= (1 << 0); }
		if ((u >= 0xFE10 && u <= 0xFE1F) || (u >= 0xFE30 && u <= 0xFE4F)) { u3 |= (1 << 1); }
		if ((u >= 0xFE50 && u <= 0xFE6F)) { u3 |= (1 << 2); }
		if ((u >= 0xFE70 && u <= 0xFEFF)) { u3 |= (1 << 3); }
		if ((u >= 0xFF00 && u <= 0xFFEF)) { u3 |= (1 << 4); }
		if ((u >= 0xFFF0 && u <= 0xFFFF)) { u3 |= (1 << 5); }
		if ((u >= 0x0F00 && u <= 0x0FFF)) { u3 |= (1 << 6); }
		if ((u >= 0x0700 && u <= 0x074F)) { u3 |= (1 << 7); }
		if ((u >= 0x0780 && u <= 0x07BF)) { u3 |= (1 << 8); }
		if ((u >= 0x0D80 && u <= 0x0DFF)) { u3 |= (1 << 9); }
		if ((u >= 0x1000 && u <= 0x109F)) { u3 |= (1 << 10); }
		if ((u >= 0x1200 && u <= 0x137F) || (u >= 0x1380 && u <= 0x139F) ||
		    (u >= 0x2D80 && u <= 0x2DDF)) {
			u3 |= (1 << 11);
		}
		if ((u >= 0x13A0 && u <= 0x13FF)) { u3 |= (1 << 12); }
		if ((u >= 0x1400 && u <= 0x167F)) { u3 |= (1 << 13); }
		if ((u >= 0x1680 && u <= 0x169F)) { u3 |= (1 << 14); }
		if ((u >= 0x16A0 && u <= 0x16FF)) { u3 |= (1 << 15); }
		if ((u >= 0x1780 && u <= 0x17FF) || (u >= 0x19E0 && u <= 0x19FF)) { u3 |= (1 << 16); }
		if ((u >= 0x1800 && u <= 0x18AF)) { u3 |= (1 << 17); }
		if ((u >= 0x2800 && u <= 0x28FF)) { u3 |= (1 << 18); }
		if ((u >= 0xA000 && u <= 0xA48F) || (u >= 0xA490 && u <= 0xA4CF)) { u3 |= (1 << 19); }
		if ((u >= 0x1700 && u <= 0x171F) || (u >= 0x1720 && u <= 0x173F) ||
		    (u >= 0x1740 && u <= 0x175F) || (u >= 0x1760 && u <= 0x177F)) {
			u3 |= (1 << 20);
		}
		if ((u >= 0x10300 && u <= 0x1032F)) { u3 |= (1 << 21); }
		if ((u >= 0x10330 && u <= 0x1034F)) { u3 |= (1 << 22); }
		if ((u >= 0x10400 && u <= 0x1044F)) { u3 |= (1 << 23); }
		if ((u >= 0x1D000 && u <= 0x1D0FF) || (u >= 0x1D100 && u <= 0x1D1FF) ||
		    (u >= 0x1D200 && u <= 0x1D24F)) {
			u3 |= (1 << 24);
		}
		if ((u >= 0x1D400 && u <= 0x1D7FF)) { u3 |= (1 << 25); }
		if ((u >= 0xFF000 && u <= 0xFFFFD) || (u >= 0x100000 && u <= 0x10FFFD)) { u3 |= (1 << 26); }
		if ((u >= 0xFE00 && u <= 0xFE0F) || (u >= 0xE0100 && u <= 0xE01EF)) { u3 |= (1 << 27); }
		if ((u >= 0xE0000 && u <= 0xE007F)) { u3 |= (1 << 28); }
		if ((u >= 0x1900 && u <= 0x194F)) { u3 |= (1 << 29); }
		if ((u >= 0x1950 && u <= 0x197F)) { u3 |= (1 << 30); }
		if ((u >= 0x1980 && u <= 0x19DF)) { u3 |= (1 << 31); }
		if ((u >= 0x1A00 && u <= 0x1A1F)) { u4 |= (1 << 0); }
		if ((u >= 0x2C00 && u <= 0x2C5F)) { u4 |= (1 << 1); }
		if ((u >= 0x2D30 && u <= 0x2D7F)) { u4 |= (1 << 2); }
		if ((u >= 0x4DC0 && u <= 0x4DFF)) { u4 |= (1 << 3); }
		if ((u >= 0xA800 && u <= 0xA82F)) { u4 |= (1 << 4); }
		if ((u >= 0x10000 && u <= 0x1007F) || (u >= 0x10080 && u <= 0x100FF) ||
		    (u >= 0x10100 && u <= 0x1013F)) {
			u4 |= (1 << 5);
		}
		if ((u >= 0x10140 && u <= 0x1018F)) { u4 |= (1 << 6); }
		if ((u >= 0x10380 && u <= 0x1039F)) { u4 |= (1 << 7); }
		if ((u >= 0x103A0 && u <= 0x103DF)) { u4 |= (1 << 8); }
		if ((u >= 0x10450 && u <= 0x1047F)) { u4 |= (1 << 9); }
		if ((u >= 0x10480 && u <= 0x104AF)) { u4 |= (1 << 10); }
		if ((u >= 0x10800 && u <= 0x1083F)) { u4 |= (1 << 11); }
		if ((u >= 0x10A00 && u <= 0x10A5F)) { u4 |= (1 << 12); }
		if ((u >= 0x1D300 && u <= 0x1D35F)) { u4 |= (1 << 13); }
		if ((u >= 0x12000 && u <= 0x123FF) || (u >= 0x12400 && u <= 0x1247F)) { u4 |= (1 << 14); }
		if ((u >= 0x1D360 && u <= 0x1D37F)) { u4 |= (1 << 15); }
		if ((u >= 0x1B80 && u <= 0x1BBF)) { u4 |= (1 << 16); }
		if ((u >= 0x1C00 && u <= 0x1C4F)) { u4 |= (1 << 17); }
		if ((u >= 0x1C50 && u <= 0x1C7F)) { u4 |= (1 << 18); }
		if ((u >= 0xA880 && u <= 0xA8DF)) { u4 |= (1 << 19); }
		if ((u >= 0xA900 && u <= 0xA92F)) { u4 |= (1 << 20); }
		if ((u >= 0xA930 && u <= 0xA95F)) { u4 |= (1 << 21); }
		if ((u >= 0xAA00 && u <= 0xAA5F)) { u4 |= (1 << 22); }
		if ((u >= 0x10190 && u <= 0x101CF)) { u4 |= (1 << 23); }
		if ((u >= 0x101D0 && u <= 0x101FF)) { u4 |= (1 << 24); }
		if ((u >= 0x102A0 && u <= 0x102DF) || (u >= 0x10280 && u <= 0x1029F) ||
		    (u >= 0x10920 && u <= 0x1093F)) {
			u4 |= (1 << 25);
		}
		if ((u >= 0x1F030 && u <= 0x1F09F) || (u >= 0x1F000 && u <= 0x1F02F)) { u4 |= (1 << 26); }
	}
	if (!options->keep_unicode_ranges) {
		font->OS_2->ulUnicodeRange1 = u1;
		font->OS_2->ulUnicodeRange2 = u2;
		font->OS_2->ulUnicodeRange3 = u3;
		font->OS_2->ulUnicodeRange4 = u4;
	}
	if (minUnicode < 0x10000) {
		font->OS_2->usFirstCharIndex = minUnicode;
	} else {
		font->OS_2->usFirstCharIndex = 0xFFFF;
	}
	if (maxUnicode < 0x10000) {
		font->OS_2->usLastCharIndex = maxUnicode;
	} else {
		font->OS_2->usLastCharIndex = 0xFFFF;
	}
}
static void statOS_2AverageWidth(otfcc_Font *font, const otfcc_Options *options) {
	if (options->keep_average_char_width) return;
	uint32_t totalWidth = 0;
	for (glyphid_t j = 0; j < font->glyf->length; j++) {
		const pos_t adw = iVQ.getStill(font->glyf->items[j]->advanceWidth);
		if (adw > 0) { totalWidth += adw; }
	}
	font->OS_2->xAvgCharWidth = totalWidth / font->glyf->length;
}
static uint16_t statMaxContextOTL(const table_OTL *table) {
	uint16_t maxc = 1;
	foreach (otl_Lookup **lookup_, table->lookups) {
		otl_Lookup *lookup = *lookup_;
		switch (lookup->type) {
			case otl_type_gpos_pair:
			case otl_type_gpos_markToBase:
			case otl_type_gpos_markToLigature:
			case otl_type_gpos_markToMark:
				if (maxc < 2) maxc = 2;
				break;
			case otl_type_gsub_ligature:
				foreach (otl_Subtable **subtable_, lookup->subtables) {
					subtable_gsub_ligature *subtable = (subtable_gsub_ligature *)*subtable_;
					foreach (otl_GsubLigatureEntry *entry, *subtable) {
						if (maxc < entry->from->numGlyphs) { maxc = entry->from->numGlyphs; }
					};
				}
				break;
			case otl_type_gsub_chaining:
			case otl_type_gpos_chaining:
				foreach (otl_Subtable **subtable_, lookup->subtables) {
					subtable_chaining *subtable = (subtable_chaining *)*subtable_;
					if (maxc < subtable->rule.matchCount) maxc = subtable->rule.matchCount;
				}
				break;
			case otl_type_gsub_reverse:
				foreach (otl_Subtable **subtable_, lookup->subtables) {
					subtable_gsub_reverse *subtable = (subtable_gsub_reverse *)*subtable_;
					if (maxc < subtable->matchCount) maxc = subtable->matchCount;
				}
				break;
			default:;
		}
	}
	return maxc;
}
static void statMaxContext(otfcc_Font *font, const otfcc_Options *options) {
	uint16_t maxc = 1;
	if (font->GSUB) {
		uint16_t maxc_gsub = statMaxContextOTL(font->GSUB);
		if (maxc_gsub > maxc) maxc = maxc_gsub;
	}
	if (font->GPOS) {
		uint16_t maxc_gpos = statMaxContextOTL(font->GPOS);
		if (maxc_gpos > maxc) maxc = maxc_gpos;
	}
	font->OS_2->usMaxContext = maxc;
}
static void statOS_2(otfcc_Font *font, const otfcc_Options *options) {
	statOS_2UnicodeRanges(font, options);
	statOS_2AverageWidth(font, options);
	statMaxContext(font, options);
}

#define MAX_STAT_METRIC 4096
static void statCFFWidths(otfcc_Font *font) {
	if (!font->glyf || !font->CFF_) return;
	// Stat the most frequent character width
	uint32_t *frequency;
	NEW(frequency, MAX_STAT_METRIC);
	for (glyphid_t j = 0; j < font->glyf->length; j++) {
		uint16_t intWidth = (uint16_t)iVQ.getStill(font->glyf->items[j]->advanceWidth);
		if (intWidth < MAX_STAT_METRIC) { frequency[intWidth] += 1; }
	}
	uint16_t maxfreq = 0;
	uint16_t maxj = 0;
	for (uint16_t j = 0; j < MAX_STAT_METRIC; j++) {
		if (frequency[j] > maxfreq) {
			maxfreq = frequency[j];
			maxj = j;
		}
	}
	// stat nominalWidthX
	uint16_t nn = 0;
	uint32_t nnsum = 0;
	for (glyphid_t j = 0; j < font->glyf->length; j++) {
		const pos_t adw = iVQ.getStill(font->glyf->items[j]->advanceWidth);
		if (adw != maxj) { nn += 1, nnsum += adw; }
	}
	int16_t nominalWidthX = 0;
	if (nn > 0) nominalWidthX = nnsum / nn;
	if (font->CFF_->privateDict) {
		font->CFF_->privateDict->defaultWidthX = maxj;
		if (nn != 0) { font->CFF_->privateDict->nominalWidthX = nominalWidthX; }
	}
	if (font->CFF_->fdArray) {
		for (tableid_t j = 0; j < font->CFF_->fdArrayCount; j++) {
			font->CFF_->fdArray[j]->privateDict->defaultWidthX = maxj;
			font->CFF_->fdArray[j]->privateDict->nominalWidthX = nominalWidthX;
		}
	}
	FREE(frequency);
}

static void statVORG(otfcc_Font *font) {
	if (!font->glyf || !font->CFF_ || !font->vhea || !font->vmtx) return;
	uint32_t *frequency;
	NEW(frequency, MAX_STAT_METRIC);
	for (glyphid_t j = 0; j < font->glyf->length; j++) {
		const pos_t vori = iVQ.getStill(font->glyf->items[j]->verticalOrigin);
		if (vori >= 0 && vori < MAX_STAT_METRIC) { frequency[(uint16_t)(vori)] += 1; }
	}
	// stat VORG.defaultVerticalOrigin
	uint32_t maxfreq = 0;
	glyphid_t maxj = 0;
	for (glyphid_t j = 0; j < MAX_STAT_METRIC; j++) {
		if (frequency[j] > maxfreq) {
			maxfreq = frequency[j];
			maxj = j;
		}
	}

	table_VORG *vorg;
	NEW(vorg);
	vorg->defaultVerticalOrigin = maxj;

	glyphid_t nVertOrigs = 0;
	for (glyphid_t j = 0; j < font->glyf->length; j++) {
		const pos_t vori = iVQ.getStill(font->glyf->items[j]->verticalOrigin);
		if (vori != maxj) { nVertOrigs += 1; }
	}
	vorg->numVertOriginYMetrics = nVertOrigs;
	NEW(vorg->entries, nVertOrigs);

	glyphid_t jj = 0;
	for (glyphid_t j = 0; j < font->glyf->length; j++) {
		const pos_t vori = iVQ.getStill(font->glyf->items[j]->verticalOrigin);
		if (vori != maxj) {
			vorg->entries[jj].gid = j;
			vorg->entries[jj].verticalOrigin = vori;
			jj += 1;
		}
	}
	FREE(frequency);
	font->VORG = vorg;
}

static void statLTSH(otfcc_Font *font) {
	if (!font->glyf) return;
	bool needLTSH = false;
	for (glyphid_t j = 0; j < font->glyf->length; j++)
		if (font->glyf->items[j]->yPel > 1) { needLTSH = true; }
	if (!needLTSH) return;

	table_LTSH *ltsh;
	NEW(ltsh);
	ltsh->numGlyphs = font->glyf->length;
	NEW(ltsh->yPels, ltsh->numGlyphs);
	for (glyphid_t j = 0; j < font->glyf->length; j++) {
		ltsh->yPels[j] = font->glyf->items[j]->yPel;
	}
	font->LTSH = ltsh;
}

void otfcc_statFont(otfcc_Font *font, const otfcc_Options *options) {
	if (font->glyf && font->head) {
		statGlyf(font, options);
		if (!options->keep_modified_time) {
			font->head->modified = 2082844800 + (int64_t)time(NULL);
		}
	}
	if (font->head && font->CFF_) {
		table_CFF *cff = font->CFF_;
		if (cff->fontBBoxBottom > font->head->yMin) cff->fontBBoxBottom = font->head->yMin;
		if (cff->fontBBoxTop < font->head->yMax) cff->fontBBoxTop = font->head->yMax;
		if (cff->fontBBoxLeft < font->head->xMin) cff->fontBBoxLeft = font->head->xMin;
		if (cff->fontBBoxRight < font->head->xMax) cff->fontBBoxRight = font->head->xMax;
		if (font->glyf && cff->isCID) { cff->cidCount = (uint32_t)font->glyf->length; }
		if (cff->isCID) {
			if (cff->fontMatrix) {
				iVQ.dispose(&cff->fontMatrix->x);
				iVQ.dispose(&cff->fontMatrix->y);
				FREE(cff->fontMatrix);
				cff->fontMatrix = NULL;
			}
			for (tableid_t j = 0; j < cff->fdArrayCount; j++) {
				table_CFF *fd = cff->fdArray[j];
				if (fd->fontMatrix) {
					iVQ.dispose(&fd->fontMatrix->x);
					iVQ.dispose(&fd->fontMatrix->y);
					FREE(fd->fontMatrix);
					fd->fontMatrix = NULL;
				}
				if (font->head->unitsPerEm == 1000) {
					fd->fontMatrix = NULL;
				} else {
					NEW(fd->fontMatrix);
					fd->fontMatrix->a = 1.0 / font->head->unitsPerEm;
					fd->fontMatrix->b = 0.0;
					fd->fontMatrix->c = 0.0;
					fd->fontMatrix->d = 1.0 / font->head->unitsPerEm;
					fd->fontMatrix->x = iVQ.neutral();
					fd->fontMatrix->y = iVQ.neutral();
				}
			}
		} else {
			if (font->head->unitsPerEm == 1000) {
				cff->fontMatrix = NULL;
			} else {
				NEW(cff->fontMatrix);
				cff->fontMatrix->a = 1.0 / font->head->unitsPerEm;
				cff->fontMatrix->b = 0.0;
				cff->fontMatrix->c = 0.0;
				cff->fontMatrix->d = 1.0 / font->head->unitsPerEm;
				cff->fontMatrix->x = iVQ.neutral();
				cff->fontMatrix->y = iVQ.neutral();
			}
		}

		statCFFWidths(font);
	}
	if (font->glyf && font->maxp) { font->maxp->numGlyphs = (uint16_t)font->glyf->length; }
	if (font->glyf && font->post) { font->post->maxMemType42 = (uint32_t)font->glyf->length; }
	if (font->glyf && font->maxp && font->maxp->version == 0x10000) {
		statMaxp(font);
		if (font->fpgm && font->fpgm->length > font->maxp->maxSizeOfInstructions) {
			font->maxp->maxSizeOfInstructions = font->fpgm->length;
		}
		if (font->prep && font->prep->length > font->maxp->maxSizeOfInstructions) {
			font->maxp->maxSizeOfInstructions = font->prep->length;
		}
	}
	if (font->OS_2 && font->cmap && font->glyf) statOS_2(font, options);
	if (font->subtype == FONTTYPE_TTF) {
		if (font->maxp) font->maxp->version = 0x00010000;
	} else {
		if (font->maxp) font->maxp->version = 0x00005000;
	}
	if (font->glyf && font->hhea) { statHmtx(font, options); }
	if (font->glyf && font->vhea) { statVmtx(font, options), statVORG(font); }
	statLTSH(font);
}

void otfcc_unstatFont(otfcc_Font *font, const otfcc_Options *options) {
	otfcc_iFont.deleteTable(font, 'hdmx');
	otfcc_iFont.deleteTable(font, 'hmtx');
	otfcc_iFont.deleteTable(font, 'VORG');
	otfcc_iFont.deleteTable(font, 'vmtx');
	otfcc_iFont.deleteTable(font, 'LTSH');
}
