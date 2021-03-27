#ifndef CARYLL_INCLUDE_TABLE_GEDF_H
#define CARYLL_INCLUDE_TABLE_GEDF_H

#include "otl.h"

typedef struct {
	int8_t format;
	pos_t coordiante;
	int16_t pointIndex;
} otl_CaretValue;
extern caryll_ElementInterface(otl_CaretValue) otl_iCaretValue;
typedef caryll_Vector(otl_CaretValue) otl_CaretValueList;
extern caryll_VectorInterface(otl_CaretValueList, otl_CaretValue) otl_iCaretValueList;

typedef struct {
	otfcc_GlyphHandle glyph;
	OWNING otl_CaretValueList carets;
} otl_CaretValueRecord;
extern caryll_ElementInterface(otl_CaretValueRecord) otl_iCaretValueRecord;
typedef caryll_Vector(otl_CaretValueRecord) otl_LigCaretTable;
extern caryll_VectorInterface(otl_LigCaretTable, otl_CaretValueRecord) otl_iLigCaretTable;

typedef struct {
	OWNING otl_ClassDef *glyphClassDef;
	OWNING otl_ClassDef *markAttachClassDef;
	OWNING otl_LigCaretTable ligCarets;
} table_GDEF;

extern caryll_RefElementInterface(table_GDEF) table_iGDEF;

#endif
