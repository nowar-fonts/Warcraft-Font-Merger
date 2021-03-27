#ifndef CARYLL_INCLUDE_TABLE_TSI_H
#define CARYLL_INCLUDE_TABLE_TSI_H

#include "table-common.h"

// TSI entries
typedef enum { TSI_GLYPH, TSI_FPGM, TSI_PREP, TSI_CVT, TSI_RESERVED_FFFC } tsi_EntryType;

typedef struct {
	tsi_EntryType type;
	otfcc_GlyphHandle glyph;
	sds content;
} tsi_Entry;

extern caryll_ElementInterface(tsi_Entry) tsi_iEntry;
typedef caryll_Vector(tsi_Entry) table_TSI;
extern caryll_VectorInterface(table_TSI, tsi_Entry) table_iTSI;

#endif
