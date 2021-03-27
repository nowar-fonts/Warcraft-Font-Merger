#ifndef CARYLL_INCLUDE_TABLE_META_H
#define CARYLL_INCLUDE_TABLE_META_H

#include "table-common.h"

// Layer and layer vector
typedef struct {
	uint32_t tag;
	sds data;
} meta_Entry;
extern caryll_ElementInterface(meta_Entry) meta_iEntry;
typedef caryll_Vector(meta_Entry) meta_Entries;
extern caryll_VectorInterface(meta_Entries, meta_Entry) meta_iEntries;

// glyph-to-layers mapping and COLR table
typedef struct {
	uint32_t version;
	uint32_t flags;
	meta_Entries entries;
} table_meta;
extern caryll_RefElementInterface(table_meta) table_iMeta;

#endif
