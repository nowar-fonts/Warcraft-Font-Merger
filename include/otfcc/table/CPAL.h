#ifndef CARYLL_INCLUDE_TABLE_CPAL_H
#define CARYLL_INCLUDE_TABLE_CPAL_H

#include "table-common.h"

// Color and Color list
typedef struct {
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha;
	uint16_t label;
} cpal_Color;
extern caryll_ElementInterface(cpal_Color) cpal_iColor;
typedef caryll_Vector(cpal_Color) cpal_ColorSet;
extern caryll_VectorInterface(cpal_ColorSet, cpal_Color) cpal_iColorSet;

// Palette and Palette list
typedef struct {
	cpal_ColorSet colorset;
	uint32_t type;
	uint32_t label;
} cpal_Palette;
extern caryll_ElementInterface(cpal_Palette) cpal_iPalette;
typedef caryll_Vector(cpal_Palette) cpal_PaletteSet;
extern caryll_VectorInterface(cpal_PaletteSet, cpal_Palette) cpal_iPaletteSet;

// CPAL table
typedef struct {
	uint16_t version;
	cpal_PaletteSet palettes;
} table_CPAL;
extern caryll_RefElementInterface(table_CPAL) table_iCPAL;

#endif
