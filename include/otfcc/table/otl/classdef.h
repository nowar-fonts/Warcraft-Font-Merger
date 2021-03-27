#ifndef CARYLL_INCLUDE_TABLE_OTL_CLASSDEF_H
#define CARYLL_INCLUDE_TABLE_OTL_CLASSDEF_H
#include "../table-common.h"
#include "coverage.h"

typedef struct {
	glyphid_t numGlyphs;
	uint32_t capacity;
	glyphclass_t maxclass;
	otfcc_GlyphHandle *glyphs;
	glyphclass_t *classes;
} otl_ClassDef;

struct __otfcc_IClassDef {
	caryll_RT(otl_ClassDef);
	void (*push)(otl_ClassDef *cd, MOVE otfcc_GlyphHandle h, glyphclass_t cls);

	otl_ClassDef *(*read)(const uint8_t *data, uint32_t tableLength, uint32_t offset);
	otl_ClassDef *(*expand)(otl_Coverage *cov, otl_ClassDef *ocd);
	json_value *(*dump)(const otl_ClassDef *cd);
	otl_ClassDef *(*parse)(const json_value *_cd);
	caryll_Buffer *(*build)(const otl_ClassDef *cd);
	void (*shrink)(otl_ClassDef *cd);
};

extern const struct __otfcc_IClassDef otl_iClassDef;

#endif
