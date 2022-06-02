#ifndef CARYLL_TABLE_GLYF_H
#define CARYLL_TABLE_GLYF_H

#include "otfcc/table/glyf.h"

glyf_Glyph *otfcc_newGlyf_glyph();
void otfcc_initGlyfContour(glyf_Contour *contour);

typedef struct {
	bool locaIsLong;
	glyphid_t numGlyphs;
	shapeid_t nPhantomPoints;
	table_fvar *fvar;
	bool hasVerticalMetrics;
	bool exportFDSelect;
} GlyfIOContext;

table_glyf *otfcc_readGlyf(const otfcc_Packet packet, const otfcc_Options *options,
                           const GlyfIOContext *ctx);
void otfcc_dumpGlyf(const table_glyf *table, json_value *root, const otfcc_Options *options,
                    const GlyfIOContext *ctx);
table_glyf *otfcc_parseGlyf(const json_value *root, otfcc_GlyphOrder *glyph_order,
                            const otfcc_Options *options);

typedef struct {
	caryll_Buffer *glyf;
	caryll_Buffer *loca;
} table_GlyfAndLocaBuffers;

table_GlyfAndLocaBuffers otfcc_buildGlyf(const table_glyf *table, table_head *head,
                                         const otfcc_Options *options);

typedef enum {
	GLYF_FLAG_ON_CURVE = 1,
	GLYF_FLAG_X_SHORT = (1 << 1),
	GLYF_FLAG_Y_SHORT = (1 << 2),
	GLYF_FLAG_REPEAT = (1 << 3),
	GLYF_FLAG_SAME_X = (1 << 4),
	GLYF_FLAG_SAME_Y = (1 << 5),
	GLYF_FLAG_POSITIVE_X = (1 << 4),
	GLYF_FLAG_POSITIVE_Y = (1 << 5)
} glyf_point_flag;

typedef enum {
	ARG_1_AND_2_ARE_WORDS = (1 << 0),
	ARGS_ARE_XY_VALUES = (1 << 1),
	ROUND_XY_TO_GRID = (1 << 2),
	WE_HAVE_A_SCALE = (1 << 3),
	MORE_COMPONENTS = (1 << 5),
	WE_HAVE_AN_X_AND_Y_SCALE = (1 << 6),
	WE_HAVE_A_TWO_BY_TWO = (1 << 7),
	WE_HAVE_INSTRUCTIONS = (1 << 8),
	USE_MY_METRICS = (1 << 9),
	OVERLAP_COMPOUND = (1 << 10),
	SCALED_COMPONENT_OFFSET = (1 << 11),
	UNSCALED_COMPONENT_OFFSET = (1 << 12)
} glyf_reference_flag;

typedef enum { MASK_ON_CURVE = 1 } glyf_oncurve_mask;

#endif
