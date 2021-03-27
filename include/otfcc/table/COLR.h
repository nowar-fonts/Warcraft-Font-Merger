#ifndef CARYLL_INCLUDE_TABLE_COLR_H
#define CARYLL_INCLUDE_TABLE_COLR_H

#include "table-common.h"

// Layer and layer vector
typedef struct {
	otfcc_GlyphHandle glyph;
	colorid_t paletteIndex;
} colr_Layer;
extern caryll_ElementInterface(colr_Layer) colr_iLayer;
typedef caryll_Vector(colr_Layer) colr_LayerList;
extern caryll_VectorInterface(colr_LayerList, colr_Layer) colr_iLayerList;

// glyph-to-layers mapping and COLR table
typedef struct {
	otfcc_GlyphHandle glyph;
	colr_LayerList layers;
} colr_Mapping;
extern caryll_ElementInterface(colr_Mapping) colr_iMapping;
typedef caryll_Vector(colr_Mapping) table_COLR;
extern caryll_VectorInterface(table_COLR, colr_Mapping) table_iCOLR;

#endif
