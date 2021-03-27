#ifndef CARYLL_INCLUDE_TABLE_CMAP_H
#define CARYLL_INCLUDE_TABLE_CMAP_H

#include "table-common.h"

// We will support format 0, 4, 12 and 14 of CMAP only
typedef struct {
	UT_hash_handle hh;
	int unicode;
	otfcc_GlyphHandle glyph;
} cmap_Entry;

typedef struct {
	uint32_t unicode;
	uint32_t selector;
} cmap_UVS_key;

typedef struct {
	UT_hash_handle hh;
	cmap_UVS_key key;
	otfcc_GlyphHandle glyph;
} cmap_UVS_Entry;

typedef struct {
	OWNING cmap_Entry *unicodes;
	OWNING cmap_UVS_Entry *uvs;
} table_cmap;

extern caryll_ElementInterfaceOf(table_cmap) {
	caryll_RT(table_cmap);
	// unicode
	bool (*encodeByIndex)(table_cmap * cmap, int c, uint16_t gid);
	bool (*encodeByName)(table_cmap * cmap, int c, MOVE sds name);
	bool (*unmap)(table_cmap * cmap, int c);
	otfcc_GlyphHandle *(*lookup)(const table_cmap *cmap, int c);
	// uvs
	bool (*encodeUVSByIndex)(table_cmap * cmap, cmap_UVS_key c, uint16_t gid);
	bool (*encodeUVSByName)(table_cmap * cmap, cmap_UVS_key c, MOVE sds name);
	bool (*unmapUVS)(table_cmap * cmap, cmap_UVS_key c);
	otfcc_GlyphHandle *(*lookupUVS)(const table_cmap *cmap, cmap_UVS_key c);
}
table_iCmap;

#endif
