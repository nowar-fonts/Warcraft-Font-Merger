#ifndef CARYLL_INCLUDE_TABLE_POST_H
#define CARYLL_INCLUDE_TABLE_POST_H

#include "table-common.h"

typedef struct {
	// PostScript information
	f16dot16 version;
	f16dot16 italicAngle;
	int16_t underlinePosition;
	int16_t underlineThickness;
	uint32_t isFixedPitch;
	uint32_t minMemType42;
	uint32_t maxMemType42;
	uint32_t minMemType1;
	uint32_t maxMemType1;
	OWNING otfcc_GlyphOrder *post_name_map;
} table_post;

extern caryll_RefElementInterface(table_post) iTable_post;

#endif
