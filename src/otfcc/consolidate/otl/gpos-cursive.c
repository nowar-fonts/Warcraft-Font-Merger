#include "gpos-cursive.h"

typedef struct {
	int fromid;
	sds fromname;
	otl_Anchor enter;
	otl_Anchor exit;
	UT_hash_handle hh;
} gpos_cursive_hash;
static int gpos_cursive_by_from_id(gpos_cursive_hash *a, gpos_cursive_hash *b) {
	return a->fromid - b->fromid;
}
bool consolidate_gpos_cursive(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                              const otfcc_Options *options) {
	subtable_gpos_cursive *subtable = &(_subtable->gpos_cursive);
	gpos_cursive_hash *h = NULL;
	for (glyphid_t k = 0; k < subtable->length; k++) {
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &subtable->items[k].target)) {
			logWarning("[Consolidate] Ignored missing glyph /%s.\n", subtable->items[k].target.name);
			continue;
		}

		gpos_cursive_hash *s;
		int fromid = subtable->items[k].target.index;
		HASH_FIND_INT(h, &fromid, s);
		if (s) {
			logWarning("[Consolidate] Double-mapping a glyph in a cursive positioning /%s.\n",
			           subtable->items[k].target.name);
		} else {
			NEW(s);
			s->fromid = subtable->items[k].target.index;
			s->fromname = sdsdup(subtable->items[k].target.name);
			s->enter = subtable->items[k].enter;
			s->exit = subtable->items[k].exit;
			HASH_ADD_INT(h, fromid, s);
		}
	}

	HASH_SORT(h, gpos_cursive_by_from_id);
	iSubtable_gpos_cursive.clear(subtable);

	gpos_cursive_hash *s, *tmp;
	HASH_ITER(hh, h, s, tmp) {
		iSubtable_gpos_cursive.push(
		    subtable, ((otl_GposCursiveEntry){
		                  .target = Handle.fromConsolidated(s->fromid, s->fromname), .enter = s->enter, .exit = s->exit,
		              }));
		sdsfree(s->fromname);
		HASH_DEL(h, s);
		FREE(s);
	}

	return (subtable->length == 0);
}
