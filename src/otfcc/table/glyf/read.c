#include "../glyf.h"

#include "support/util.h"
#include "support/ttinstr/ttinstr.h"

static glyf_Point *next_point(glyf_ContourList *contours, shapeid_t *cc, shapeid_t *cp) {
	if (*cp >= contours->items[*cc].length) {
		*cp = 0;
		*cc += 1;
	}
	return &contours->items[*cc].items[(*cp)++];
}

static glyf_Glyph *otfcc_read_simple_glyph(font_file_pointer start, shapeid_t numberOfContours,
                                           const otfcc_Options *options) {
	glyf_Glyph *g = otfcc_newGlyf_glyph();
	glyf_ContourList *contours = &g->contours;

	shapeid_t pointsInGlyph = 0;
	for (shapeid_t j = 0; j < numberOfContours; j++) {
		shapeid_t lastPointInCurrentContour = read_16u(start + 2 * j);
		glyf_Contour contour;
		glyf_iContour.init(&contour);
		glyf_iContour.fill(&contour, lastPointInCurrentContour - pointsInGlyph + 1);
		glyf_iContourList.push(contours, contour);
		pointsInGlyph = lastPointInCurrentContour + 1;
	}
	uint16_t instructionLength = read_16u(start + 2 * numberOfContours);
	uint8_t *instructions = NULL;
	if (instructionLength > 0) {
		NEW(instructions, instructionLength);
		memcpy(instructions, start + 2 * numberOfContours + 2, sizeof(uint8_t) * instructionLength);
	}
	g->instructionsLength = instructionLength;
	g->instructions = instructions;

	// read flags
	// There are repeating entries in the flags list, we will fill out the
	// result
	font_file_pointer flags;
	NEW(flags, pointsInGlyph);
	font_file_pointer flagStart = start + 2 * numberOfContours + 2 + instructionLength;
	shapeid_t flagsReadSofar = 0;
	shapeid_t flagBytesReadSofar = 0;

	shapeid_t currentContour = 0;
	shapeid_t currentContourPointIndex = 0;
	while (flagsReadSofar < pointsInGlyph) {
		uint8_t flag = flagStart[flagBytesReadSofar];
		flags[flagsReadSofar] = flag;
		flagBytesReadSofar += 1;
		flagsReadSofar += 1;
		next_point(contours, &currentContour, &currentContourPointIndex)->onCurve =
		    (flag & GLYF_FLAG_ON_CURVE);
		if (flag & GLYF_FLAG_REPEAT) { // repeating flag
			uint8_t repeat = flagStart[flagBytesReadSofar];
			flagBytesReadSofar += 1;
			for (uint8_t j = 0; j < repeat; j++) {
				flags[flagsReadSofar + j] = flag;
				next_point(contours, &currentContour, &currentContourPointIndex)->onCurve =
				    (flag & GLYF_FLAG_ON_CURVE);
			}
			flagsReadSofar += repeat;
		}
	}

	// read X coordinates
	font_file_pointer coordinatesStart = flagStart + flagBytesReadSofar;
	uint32_t coordinatesOffset = 0;
	shapeid_t coordinatesRead = 0;
	currentContour = 0;
	currentContourPointIndex = 0;
	while (coordinatesRead < pointsInGlyph) {
		uint8_t flag = flags[coordinatesRead];
		int16_t x;
		if (flag & GLYF_FLAG_X_SHORT) {
			x = (flag & GLYF_FLAG_POSITIVE_X ? 1 : -1) *
			    read_8u(coordinatesStart + coordinatesOffset);
			coordinatesOffset += 1;
		} else {
			if (flag & GLYF_FLAG_SAME_X) {
				x = 0;
			} else {
				x = read_16s(coordinatesStart + coordinatesOffset);
				coordinatesOffset += 2;
			}
		}
		iVQ.replace(&(next_point(contours, &currentContour, &currentContourPointIndex)->x),
		            iVQ.createStill(x));
		coordinatesRead += 1;
	}
	// read Y, identical to X
	coordinatesRead = 0;
	currentContour = 0;
	currentContourPointIndex = 0;
	while (coordinatesRead < pointsInGlyph) {
		uint8_t flag = flags[coordinatesRead];
		int16_t y;
		if (flag & GLYF_FLAG_Y_SHORT) {
			y = (flag & GLYF_FLAG_POSITIVE_Y ? 1 : -1) *
			    read_8u(coordinatesStart + coordinatesOffset);
			coordinatesOffset += 1;
		} else {
			if (flag & GLYF_FLAG_SAME_Y) {
				y = 0;
			} else {
				y = read_16s(coordinatesStart + coordinatesOffset);
				coordinatesOffset += 2;
			}
		}
		iVQ.replace(&(next_point(contours, &currentContour, &currentContourPointIndex)->y),
		            iVQ.createStill(y));
		coordinatesRead += 1;
	}
	FREE(flags);

	// turn deltas to absolute coordiantes
	VQ cx = iVQ.neutral(), cy = iVQ.neutral();
	for (shapeid_t j = 0; j < numberOfContours; j++) {
		for (shapeid_t k = 0; k < contours->items[j].length; k++) {
			glyf_Point *z = &contours->items[j].items[k];
			iVQ.inplacePlus(&cx, z->x);
			iVQ.inplacePlus(&cy, z->y);
			iVQ.copyReplace(&z->x, cx);
			iVQ.copyReplace(&z->y, cy);
		}
		glyf_iContour.shrinkToFit(&contours->items[j]);
	}
	glyf_iContourList.shrinkToFit(contours);
	iVQ.dispose(&cx), iVQ.dispose(&cy);
	return g;
}

static glyf_Glyph *otfcc_read_composite_glyph(font_file_pointer start,
                                              const otfcc_Options *options) {
	glyf_Glyph *g = otfcc_newGlyf_glyph();

	// pass 1, read references quantity
	uint16_t flags = 0;
	uint32_t offset = 0;
	bool glyphHasInstruction = false;
	do {
		flags = read_16u(start + offset);
		glyphid_t index = read_16u(start + offset + 2);

		glyf_ComponentReference ref = glyf_iComponentReference.empty();
		ref.glyph = Handle.fromIndex(index);

		offset += 4; // flags & index
		if (flags & ARGS_ARE_XY_VALUES) {
			ref.isAnchored = REF_XY;
			if (flags & ARG_1_AND_2_ARE_WORDS) {
				ref.x = iVQ.createStill(read_16s(start + offset));
				ref.y = iVQ.createStill(read_16s(start + offset + 2));
				offset += 4;
			} else {
				ref.x = iVQ.createStill(read_8s(start + offset));
				ref.y = iVQ.createStill(read_8s(start + offset + 1));
				offset += 2;
			}
		} else {
			ref.isAnchored = REF_ANCHOR_ANCHOR;
			if (flags & ARG_1_AND_2_ARE_WORDS) {
				ref.outer = read_16u(start + offset);
				ref.inner = read_16u(start + offset + 2);
				offset += 4;
			} else {
				ref.outer = read_8u(start + offset);
				ref.inner = read_8u(start + offset + 1);
				offset += 2;
			}
		}
		if (flags & WE_HAVE_A_SCALE) {
			ref.a = ref.d = otfcc_from_f2dot14(read_16s(start + offset));
			offset += 2;
		} else if (flags & WE_HAVE_AN_X_AND_Y_SCALE) {
			ref.a = otfcc_from_f2dot14(read_16s(start + offset));
			ref.d = otfcc_from_f2dot14(read_16s(start + offset + 2));
			offset += 4;
		} else if (flags & WE_HAVE_A_TWO_BY_TWO) {
			ref.a = otfcc_from_f2dot14(read_16s(start + offset));
			ref.b = otfcc_from_f2dot14(read_16s(start + offset + 2));
			ref.c = otfcc_from_f2dot14(read_16s(start + offset + 4));
			ref.d = otfcc_from_f2dot14(read_16s(start + offset + 6));
			offset += 8;
		}
		ref.roundToGrid = flags & ROUND_XY_TO_GRID;
		ref.useMyMetrics = flags & USE_MY_METRICS;
		if (flags & SCALED_COMPONENT_OFFSET &&
		    (flags & WE_HAVE_AN_X_AND_Y_SCALE || flags & WE_HAVE_A_TWO_BY_TWO)) {
			logWarning("glyf: SCALED_COMPONENT_OFFSET is not supported.")
		}
		if (flags & WE_HAVE_INSTRUCTIONS) { glyphHasInstruction = true; }
		glyf_iReferenceList.push(&g->references, ref);
	} while (flags & MORE_COMPONENTS);

	if (glyphHasInstruction) {
		uint16_t instructionLength = read_16u(start + offset);
		font_file_pointer instructions = NULL;
		if (instructionLength > 0) {
			NEW(instructions, instructionLength);
			memcpy(instructions, start + offset + 2, sizeof(uint8_t) * instructionLength);
		}
		g->instructionsLength = instructionLength;
		g->instructions = instructions;
	} else {
		g->instructionsLength = 0;
		g->instructions = NULL;
	}

	return g;
}

static glyf_Glyph *otfcc_read_glyph(font_file_pointer data, uint32_t offset,
                                    const otfcc_Options *options) {
	font_file_pointer start = data + offset;
	int16_t numberOfContours = read_16u(start);
	glyf_Glyph *g;
	if (numberOfContours > 0) {
		g = otfcc_read_simple_glyph(start + 10, numberOfContours, options);
	} else {
		g = otfcc_read_composite_glyph(start + 10, options);
	}
	g->stat.xMin = read_16s(start + 2);
	g->stat.yMin = read_16s(start + 4);
	g->stat.xMax = read_16s(start + 6);
	g->stat.yMax = read_16s(start + 8);
	return g;
}

// common states of tuple polymorphizer

typedef struct {
	table_fvar *fvar;
	uint16_t dimensions;
	uint16_t sharedTupleCount;
	f2dot14 *sharedTuples;
	uint8_t coordDimensions;
	bool allowIUP;
	shapeid_t nPhantomPoints;
} TuplePolymorphizerCtx;

// GVAR header
#pragma pack(1)
struct GVARHeader {
	uint16_t majorVersion;
	uint16_t minorVersion;
	uint16_t axisCount;
	uint16_t sharedTupleCount;
	uint32_t sharedTuplesOffset;
	uint16_t glyphCount;
	uint16_t flags;
	uint32_t glyphVariationDataArrayOffset;
};

struct TupleVariationHeader {
	uint16_t variationDataSize;
	uint16_t tupleIndex;
};
struct GlyphVariationData {
	uint16_t tupleVariationCount;
	uint16_t dataOffset;
	struct TupleVariationHeader tvhs[];
};
#pragma pack()

#define GVAR_OFFSETS_ARE_LONG 1
#define EMBEDDED_PEAK_TUPLE 0x8000
#define INTERMEDIATE_REGION 0x4000
#define PRIVATE_POINT_NUMBERS 0x2000
#define TUPLE_INDEX_MASK 0xFFF

static INLINE struct TupleVariationHeader *nextTVH(struct TupleVariationHeader *currentHeader,
                                                   const TuplePolymorphizerCtx *ctx) {
	uint32_t bump = 2 * sizeof(uint16_t);
	uint16_t tupleIndex = be16(currentHeader->tupleIndex);
	if (tupleIndex & EMBEDDED_PEAK_TUPLE) bump += ctx->dimensions * sizeof(f2dot14);
	if (tupleIndex & INTERMEDIATE_REGION) bump += 2 * ctx->dimensions * sizeof(f2dot14);
	return (struct TupleVariationHeader *)((font_file_pointer)currentHeader + bump);
}

#define POINT_COUNT_IS_WORD 0x80
#define POINT_COUNT_LONG_MASK 0x7FFF
#define POINT_RUN_COUNT_MASK 0x7F
#define POINTS_ARE_WORDS 0x80

static INLINE font_file_pointer parsePointNumbers(font_file_pointer data, shapeid_t **pointIndeces,
                                                  shapeid_t *pc, shapeid_t totalPoints) {
	uint16_t nPoints = 0;
	uint8_t firstByte = *data;
	if (firstByte & POINT_COUNT_IS_WORD) {
		nPoints = (data[0] << 8 | data[1]) & POINT_COUNT_LONG_MASK;
		data += 2;
	} else {
		nPoints = firstByte;
		data++;
	}
	if (nPoints > 0) {
		struct {
			shapeid_t length;
			bool wide;
		} run = {0, false};
		shapeid_t filled = 0;
		shapeid_t jPoint = 0;
		NEW_CLEAN_N(*pointIndeces, nPoints);
		while (filled < nPoints) {
			if (run.length == 0) {
				uint8_t runHeader = *data++;
				run.wide = !!(runHeader & POINTS_ARE_WORDS);
				run.length = (runHeader & POINT_RUN_COUNT_MASK) + 1;
			}
			int16_t pointNumber = jPoint;
			if (run.wide) {
				pointNumber += *((uint16_t *)data);
				data += 2;
			} else {
				pointNumber += *data++;
			}
			(*pointIndeces)[filled] = pointNumber;
			filled++;
			jPoint = pointNumber;
			--run.length;
		}
		*pc = nPoints;
	} else {
		NEW_CLEAN_N(*pointIndeces, totalPoints);
		for (shapeid_t j = 0; j < totalPoints; j++) {
			(*pointIndeces)[j] = j;
		}
		*pc = totalPoints;
	}

	return data;
}

#define DELTAS_ARE_ZERO 0x80
#define DELTAS_ARE_WORDS 0x40
#define DELTA_RUN_COUNT_MASK 0x3F

static INLINE font_file_pointer readPackedDelta(font_file_pointer data, shapeid_t nPoints,
                                                pos_t *deltas) {
	struct {
		shapeid_t length;
		bool wide;
		bool zero;
	} run = {0, false, false};
	shapeid_t filled = 0;
	while (filled < nPoints) {
		int16_t delta = 0;
		if (run.length == 0) {
			uint8_t runHeader = *data++;
			run.zero = runHeader & DELTAS_ARE_ZERO;
			run.wide = runHeader & DELTAS_ARE_WORDS;
			run.length = (runHeader & DELTA_RUN_COUNT_MASK) + 1;
		}
		if (!run.zero) {
			if (run.wide) {
				delta = (int16_t)be16(*(uint16_t *)data);
				data += 2;
			} else {
				delta = (int8_t)*data;
				data++;
			}
		}
		deltas[filled] = (pos_t)delta;
		filled++;
		--run.length;
	}
	return data;
}

typedef VQ *(*CoordPartGetter)(glyf_Point *z);

VQ *getX(glyf_Point *z) {
	return &z->x;
}
VQ *getY(glyf_Point *z) {
	return &z->y;
}

static INLINE void fillTheGaps(shapeid_t jMin, shapeid_t jMax, vq_Segment *nudges,
                               glyf_Point **glyphRefs, CoordPartGetter getter) {
	for (shapeid_t j = jMin; j < jMax; j++) {
		if (nudges[j].val.delta.touched) continue;
		// get next knot
		shapeid_t jNext = j;
		while (!nudges[jNext].val.delta.touched) {
			if (jNext == jMax - 1) {
				jNext = jMin;
			} else {
				jNext += 1;
			}
			if (jNext == j) break;
		}
		// get pre knot
		shapeid_t jPrev = j;
		while (!nudges[jPrev].val.delta.touched) {
			if (jPrev == jMin) {
				jPrev = jMax - 1;
			} else {
				jPrev -= 1;
			}
			if (jPrev == j) break;
		}
		if (nudges[jNext].val.delta.touched && nudges[jPrev].val.delta.touched) {
			f16dot16 untouchJ = otfcc_to_fixed(getter(glyphRefs[j])->kernel);
			f16dot16 untouchPrev = otfcc_to_fixed(getter(glyphRefs[jPrev])->kernel);
			f16dot16 untouchNext = otfcc_to_fixed(getter(glyphRefs[jNext])->kernel);
			f16dot16 deltaPrev = otfcc_to_fixed(nudges[jPrev].val.delta.quantity);
			f16dot16 deltaNext = otfcc_to_fixed(nudges[jNext].val.delta.quantity);

			f16dot16 uMin = untouchPrev;
			f16dot16 uMax = untouchNext;
			f16dot16 dMin = deltaPrev;
			f16dot16 dMax = deltaNext;
			if (untouchPrev > untouchNext) {
				uMin = untouchNext;
				uMax = untouchPrev;
				dMin = deltaNext;
				dMax = deltaPrev;
			}
			if (untouchJ <= uMin) {
				nudges[j].val.delta.quantity = otfcc_from_fixed(dMin);
			} else if (untouchJ >= uMax) {
				nudges[j].val.delta.quantity = otfcc_from_fixed(dMax);
			} else {
				nudges[j].val.delta.quantity =
				    otfcc_from_fixed(otfcc_f1616_muldiv(dMax - dMin, untouchJ - uMin, uMax - uMin));
			}
		}
	}
}

static void applyCoords(const shapeid_t totalPoints, glyf_Glyph *glyph,
                        glyf_Point **glyphRefs, // target
                        const shapeid_t nTouchedPoints, const pos_t *tupleDelta,
                        const shapeid_t *points,
                        const vq_Region *r, // data
                        CoordPartGetter getter) {
	vq_Segment *nudges;
	NEW_CLEAN_N(nudges, totalPoints);
	for (shapeid_t j = 0; j < totalPoints; j++) {
		nudges[j].type = VQ_DELTA;
		nudges[j].val.delta.touched = false;
		nudges[j].val.delta.quantity = 0;
		nudges[j].val.delta.region = r;
	}
	for (shapeid_t j = 0; j < nTouchedPoints; j++) {
		if (points[j] >= totalPoints) continue;
		nudges[points[j]].val.delta.touched = true;
		nudges[points[j]].val.delta.quantity += tupleDelta[j];
	}
	// fill the gaps
	shapeid_t jFirst = 0;
	foreach (glyf_Contour *c, glyph->contours) {
		fillTheGaps(jFirst, jFirst + c->length, nudges, glyphRefs, getX);
		jFirst += c->length;
	}
	for (shapeid_t j = 0; j < totalPoints; j++) {
		if (!nudges[j].val.delta.quantity && nudges[j].val.delta.touched) continue;
		VQ *coordinatePart = getter(glyphRefs[j]);
		vq_iSegList.push(&(coordinatePart->shift), nudges[j]);
	}
	FREE(nudges);
}

static INLINE void applyPolymorphism(const shapeid_t totalPoints, glyf_GlyphPtr glyph, // target
                                     const shapeid_t nTouchedPoints, const shapeid_t *points,
                                     const pos_t *deltaX, const pos_t *deltaY,
                                     const vq_Region *r // delta data
) {
	glyf_Point **glyphRefs;
	NEW_CLEAN_N(glyphRefs, totalPoints);
	{
		shapeid_t j = 0;
		foreach (glyf_Contour *c, glyph->contours) {
			foreach (glyf_Point *g, *c) { glyphRefs[j++] = g; }
		}
		foreach (glyf_ComponentReference *r, glyph->references) {
			// in glyf_ComponentReference, we also have a X and a Y entry
			// so a trick of conversion
			glyphRefs[j++] = (glyf_Point *)&(r->x);
		}
	}
	applyCoords(totalPoints, glyph, glyphRefs, nTouchedPoints, deltaX, points, r, getX);
	applyCoords(totalPoints, glyph, glyphRefs, nTouchedPoints, deltaY, points, r, getY);
	// Horizontal phantom point
	if (totalPoints + 1 < nTouchedPoints) {
		iVQ.addDelta(&(glyph->horizontalOrigin), true, r, deltaX[totalPoints]);
		iVQ.addDelta(&(glyph->advanceWidth), true, r,
		             deltaX[totalPoints + 1] - deltaX[totalPoints]);
	}
	// Vertical phantom point
	if (totalPoints + 3 < nTouchedPoints) {
		iVQ.addDelta(&(glyph->verticalOrigin), true, r, deltaY[totalPoints + 2]);
		iVQ.addDelta(&(glyph->advanceHeight), true, r,
		             deltaY[totalPoints + 2] - deltaY[totalPoints + 3]);
	}

	FREE(glyphRefs);
}

static vq_Region *createRegionFromTuples(uint16_t dimensions, f2dot14 *peak, f2dot14 *start,
                                         f2dot14 *end) {
	vq_Region *r = vq_createRegion(dimensions);
	for (uint16_t d = 0; d < dimensions; d++) {
		pos_t peakVal = otfcc_from_f2dot14(be16(peak[d]));
		vq_AxisSpan span = {peakVal <= 0 ? -1 : 0, peakVal, peakVal >= 0 ? 1 : 0};
		if (start && end) {
			span.start = otfcc_from_f2dot14(be16(start[d]));
			span.end = otfcc_from_f2dot14(be16(end[d]));
		}
		r->spans[d] = span;
	}
	return r;
}

static INLINE void polymorphizeGlyph(glyphid_t gid, glyf_GlyphPtr glyph,
                                     const TuplePolymorphizerCtx *ctx,
                                     struct GlyphVariationData *gvd, const otfcc_Options *options) {

	shapeid_t totalPoints = 0;
	foreach (glyf_Contour *c, glyph->contours) { totalPoints += c->length; }
	totalPoints += glyph->references.length;
	shapeid_t totalDeltaEntries = totalPoints + ctx->nPhantomPoints;

	uint16_t nTuples = be16(gvd->tupleVariationCount) & 0xFFF;
	struct TupleVariationHeader *tvh = gvd->tvhs;

	bool hasSharedPointNumbers = be16(gvd->tupleVariationCount) & 0x8000;
	shapeid_t sharedPointCount = 0;
	shapeid_t *sharedPointIndeces = NULL;
	font_file_pointer data = (font_file_pointer)gvd + be16(gvd->dataOffset);
	if (hasSharedPointNumbers) {
		data = parsePointNumbers(data, &sharedPointIndeces, &sharedPointCount, totalDeltaEntries);
	}

	size_t tsdStart = 0;

	for (uint16_t j = 0; j < nTuples; j++) {

		// Tuple options
		shapeid_t tupleIndex = be16(tvh->tupleIndex) & TUPLE_INDEX_MASK;
		bool hasEmbeddedPeak = be16(tvh->tupleIndex) & EMBEDDED_PEAK_TUPLE;
		bool hasIntermediate = be16(tvh->tupleIndex) & INTERMEDIATE_REGION;

		// Peak tuple
		f2dot14 *peak = NULL;
		if (hasEmbeddedPeak) {
			peak = (f2dot14 *)(((font_file_pointer)tvh) + 4);
		} else {
			peak = ctx->sharedTuples + ctx->dimensions * tupleIndex;
		}

		// Intermediate tuple -- if present
		f2dot14 *start = NULL;
		f2dot14 *end = NULL;
		if (hasIntermediate) {
			start = (f2dot14 *)(((font_file_pointer)tvh) + 4 +
			                    2 * (hasEmbeddedPeak ? 1 : 0) * ctx->dimensions);
			end = (f2dot14 *)(((font_file_pointer)tvh) + 4 +
			                  2 * (hasEmbeddedPeak ? 2 : 1) * ctx->dimensions);
		}

		const vq_Region *r = table_iFvar.registerRegion(
		    ctx->fvar, createRegionFromTuples(ctx->dimensions, peak, start, end));

		// Pointer of tuple serialized data
		font_file_pointer tsd = data + tsdStart;
		// Point number term
		shapeid_t nPoints = sharedPointCount;
		shapeid_t *pointIndeces = sharedPointIndeces;

		if (be16(tvh->tupleIndex) & PRIVATE_POINT_NUMBERS) {
			shapeid_t privatePointCount = 0;
			shapeid_t *privatePointNumbers = NULL;
			tsd =
			    parsePointNumbers(tsd, &privatePointNumbers, &privatePointCount, totalDeltaEntries);
			nPoints = privatePointCount;
			pointIndeces = privatePointNumbers;
		}
		if (pointIndeces) {
			// Delta term
			pos_t *deltaX = 0;
			pos_t *deltaY = 0;
			NEW_CLEAN_N(deltaX, nPoints);
			NEW_CLEAN_N(deltaY, nPoints);

			tsd = readPackedDelta(tsd, nPoints, deltaX);
			tsd = readPackedDelta(tsd, nPoints, deltaY);

			// Do polymorphize
			applyPolymorphism(totalPoints, glyph, nPoints, pointIndeces, deltaX, deltaY, r);
			FREE(deltaX);
			FREE(deltaY);
		}
		// Cleanup
		if (be16(tvh->tupleIndex) & PRIVATE_POINT_NUMBERS) { FREE(pointIndeces); }
		tsdStart += be16(tvh->variationDataSize);

		tvh = nextTVH(tvh, ctx);
	}
	FREE(sharedPointIndeces);
}

// NOTE: for polymorphize, we would
// TODO: polymorphize advanceWidth, verticalOrigin and advanceHeight
static INLINE void polymorphize(const otfcc_Packet packet, const otfcc_Options *options,
                                table_glyf *glyf, const GlyfIOContext *ctx) {
	if (!ctx->fvar || !ctx->fvar->axes.length) return;
	FOR_TABLE('gvar', table) {
		font_file_pointer data = table.data;
		if (table.length < sizeof(struct GVARHeader)) return;
		struct GVARHeader *header = (struct GVARHeader *)data;
		if (be16(header->axisCount) != ctx->fvar->axes.length) {
			logWarning("Axes number in GVAR and FVAR are inequal");
			return;
		};
		for (glyphid_t j = 0; j < glyf->length; j++) {
			TuplePolymorphizerCtx tpctx = {.fvar = ctx->fvar,
			                               .dimensions = ctx->fvar->axes.length,
			                               .nPhantomPoints = ctx->nPhantomPoints,
			                               .sharedTupleCount = be16(header->sharedTupleCount),
			                               .sharedTuples =
			                                   (f2dot14 *)(data + be32(header->sharedTuplesOffset)),
			                               .coordDimensions = 2,
			                               .allowIUP = glyf->items[j]->contours.length > 0};
			uint32_t glyphVariationDataOffset = 0;
			if (be16(header->flags) & GVAR_OFFSETS_ARE_LONG) {
				glyphVariationDataOffset =
				    be32(((uint32_t *)(data + sizeof(struct GVARHeader)))[j]);
			} else {
				glyphVariationDataOffset =
				    2 * be16(((uint16_t *)(data + sizeof(struct GVARHeader)))[j]);
			}
			struct GlyphVariationData *gvd =
			    (struct GlyphVariationData *)(data + be32(header->glyphVariationDataArrayOffset) +
			                                  glyphVariationDataOffset);
			polymorphizeGlyph(j, glyf->items[j], &tpctx, gvd, options);
		}
	}
}

table_glyf *otfcc_readGlyf(const otfcc_Packet packet, const otfcc_Options *options,
                           const GlyfIOContext *ctx) {
	uint32_t *offsets = NULL;
	table_glyf *glyf = NULL;

	NEW_CLEAN_N(offsets, (ctx->numGlyphs + 1));
	if (!offsets) goto ABSENT;
	bool foundLoca = false;

	// read loca
	FOR_TABLE('loca', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < 2 * ctx->numGlyphs + 2) goto LOCA_CORRUPTED;
		for (uint32_t j = 0; j < ctx->numGlyphs + 1; j++) {
			if (ctx->locaIsLong) {
				offsets[j] = read_32u(data + j * 4);
			} else {
				offsets[j] = read_16u(data + j * 2) * 2;
			}
			if (j > 0 && offsets[j] < offsets[j - 1]) goto LOCA_CORRUPTED;
		}
		foundLoca = true;
		break;
	LOCA_CORRUPTED:
		logWarning("table 'loca' corrupted.\n");
		if (offsets) { FREE(offsets), offsets = NULL; }
		continue;
	}
	if (!foundLoca) goto ABSENT;

	// read glyf
	FOR_TABLE('glyf', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < offsets[ctx->numGlyphs]) goto GLYF_CORRUPTED;

		glyf = table_iGlyf.create();

		for (glyphid_t j = 0; j < ctx->numGlyphs; j++) {
			if (offsets[j] < offsets[j + 1]) { // non-space glyph
				table_iGlyf.push(glyf, otfcc_read_glyph(data, offsets[j], options));
			} else { // space glyph
				table_iGlyf.push(glyf, otfcc_newGlyf_glyph());
			}
		}
		goto PRESENT;
	GLYF_CORRUPTED:
		logWarning("table 'glyf' corrupted.\n");
		if (glyf) { DELETE(table_iGlyf.free, glyf), glyf = NULL; }
	}
	goto ABSENT;

PRESENT:
	if (offsets) { FREE(offsets), offsets = NULL; }
	// We have glyf table read. Do polymorphize
	polymorphize(packet, options, glyf, ctx);
	return glyf;

ABSENT:
	if (offsets) { FREE(offsets), offsets = NULL; }
	if (glyf) { FREE(glyf), glyf = NULL; }
	return NULL;
}
