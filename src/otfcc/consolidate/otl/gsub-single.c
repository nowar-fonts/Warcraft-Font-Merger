#include "gsub-single.h"

typedef struct {
	int fromid;
	sds fromname;
	int toid;
	sds toname;
	UT_hash_handle hh;
} gsub_single_map_hash;
static int by_from_id(gsub_single_map_hash *a, gsub_single_map_hash *b) {
	return a->fromid - b->fromid;
}
bool consolidate_gsub_single(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                             const otfcc_Options *options) {
	subtable_gsub_single *subtable = &(_subtable->gsub_single);
	gsub_single_map_hash *h = NULL;
	for (size_t k = 0; k < subtable->length; k++) {
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &subtable->items[k].from)) {
			logWarning("[Consolidate] Ignored missing glyph /%s.\n", subtable->items[k].from.name);
			continue;
		}
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &subtable->items[k].to)) {
			logWarning("[Consolidate] Ignored missing glyph /%s.\n", subtable->items[k].to.name);
			continue;
		}
		gsub_single_map_hash *s;
		int fromid = subtable->items[k].from.index;
		HASH_FIND_INT(h, &fromid, s);
		if (s) {
			logWarning("[Consolidate] Double-mapping a glyph in a single substitution /%s.\n",
			           subtable->items[k].from.name);
		} else {
			NEW(s);
			s->fromid = subtable->items[k].from.index;
			s->toid = subtable->items[k].to.index;
			s->fromname = sdsdup(subtable->items[k].from.name);
			s->toname = sdsdup(subtable->items[k].to.name);
			HASH_ADD_INT(h, fromid, s);
		}
	}
	HASH_SORT(h, by_from_id);
	if (HASH_COUNT(h) != subtable->length) { logWarning("[Consolidate] In this lookup, some mappings are ignored.\n"); }

	iSubtable_gsub_single.clear(subtable);
	gsub_single_map_hash *s, *tmp;
	HASH_ITER(hh, h, s, tmp) {
		iSubtable_gsub_single.push(subtable,
		                           ((otl_GsubSingleEntry){.from = Handle.fromConsolidated(s->fromid, s->fromname),
		                                                  .to = Handle.fromConsolidated(s->toid, s->toname)}));
		sdsfree(s->fromname);
		sdsfree(s->toname);
		HASH_DEL(h, s);
		FREE(s);
	}
	return (subtable->length == 0);
}
