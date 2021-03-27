#ifndef CARYLL_INCLUDE_TABLE_BASE_H
#define CARYLL_INCLUDE_TABLE_BASE_H

#include "otl.h"

typedef struct {
	uint32_t tag;
	pos_t coordinate;
} otl_BaseValue;

typedef struct {
	uint32_t tag;
	uint32_t defaultBaselineTag;
	tableid_t baseValuesCount;
	OWNING otl_BaseValue *baseValues;
} otl_BaseScriptEntry;

typedef struct {
	tableid_t scriptCount;
	otl_BaseScriptEntry *entries;
} otl_BaseAxis;

typedef struct {
	otl_BaseAxis *horizontal;
	otl_BaseAxis *vertical;
} table_BASE;

extern caryll_RefElementInterface(table_BASE) table_iBASE;

#endif
