#ifndef CARYLL_INCLUDE_TABLE_VORG_H
#define CARYLL_INCLUDE_TABLE_VORG_H

#include "table-common.h"

typedef struct {
	glyphid_t gid;
	int16_t verticalOrigin;
} VORG_entry;

typedef struct {
	glyphid_t numVertOriginYMetrics;
	pos_t defaultVerticalOrigin;
	OWNING VORG_entry *entries;
} table_VORG;

extern caryll_RefElementInterface(table_VORG) table_iVORG;

#endif
