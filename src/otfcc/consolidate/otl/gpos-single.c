#include "gpos-single.h"

typedef struct {
	int fromid;
	sds fromname;
	otl_PositionValue v;
	UT_hash_handle hh;
} gpos_single_hash;
static int gpos_by_from_id(gpos_single_hash *a, gpos_single_hash *b) {
	return a->fromid - b->fromid;
}

bool consolidate_gpos_single(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                             const otfcc_Options *options) {
	subtable_gpos_single *subtable = &(_subtable->gpos_single);
	gpos_single_hash *h = NULL;
	for (glyphid_t k = 0; k < subtable->length; k++) {
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &subtable->items[k].target)) {
			logWarning("[Consolidate] Ignored missing glyph /%s.\n",
			           subtable->items[k].target.name);
			continue;
		}
		gpos_single_hash *s;
		int fromid = subtable->items[k].target.index;
		HASH_FIND_INT(h, &fromid, s);
		if (s) {
			logWarning("[Consolidate] Detected glyph double-mapping about /%s.\n",
			           subtable->items[k].target.name);
		} else {
			NEW(s);
			s->fromid = subtable->items[k].target.index;
			s->fromname = sdsdup(subtable->items[k].target.name);
			s->v = subtable->items[k].value;
			HASH_ADD_INT(h, fromid, s);
		}
	}

	HASH_SORT(h, gpos_by_from_id);
	iSubtable_gpos_single.clear(subtable);

	gpos_single_hash *s, *tmp;
	HASH_ITER(hh, h, s, tmp) {
		iSubtable_gpos_single.push(
		    subtable, ((otl_GposSingleEntry){
		                  .target = Handle.fromConsolidated(s->fromid, s->fromname), .value = s->v,
		              }));
		sdsfree(s->fromname);
		HASH_DEL(h, s);
		FREE(s);
	}

	return (subtable->length == 0);
}
