#ifndef CARYLL_INCLUDE_TABLE_SVG_H
#define CARYLL_INCLUDE_TABLE_SVG_H

#include "table-common.h"

// SVG assignment
typedef struct {
	// Due to the outward references from SVG document to GID.
	// We have to use GID here. Sorry.
	glyphid_t start;
	glyphid_t end;
	caryll_Buffer *document;
} svg_Assignment;
extern caryll_ValElementInterface(svg_Assignment) svg_iAssignment;
typedef caryll_Vector(svg_Assignment) table_SVG;
extern caryll_VectorInterface(table_SVG, svg_Assignment) table_iSVG;

#endif
