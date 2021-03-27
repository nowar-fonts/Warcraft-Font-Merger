#include "gsub-reverse.h"

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

bool consolidate_gsub_reverse(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                              const otfcc_Options *options) {
	subtable_gsub_reverse *subtable = &(_subtable->gsub_reverse);
	for (tableid_t j = 0; j < subtable->matchCount; j++) {
		fontop_consolidateCoverage(font, subtable->match[j], options);
	}
	fontop_consolidateCoverage(font, subtable->to, options);
	if (subtable->inputIndex >= subtable->matchCount) { subtable->inputIndex = subtable->matchCount - 1; }
	gsub_single_map_hash *h = NULL;
	otl_Coverage *from = subtable->match[subtable->inputIndex];
	for (glyphid_t k = 0; k < from->numGlyphs && k < subtable->to->numGlyphs; k++) {
		gsub_single_map_hash *s;
		int fromid = from->glyphs[k].index;
		HASH_FIND_INT(h, &fromid, s);
		if (s) {
			logWarning("[Consolidate] Double-mapping a glyph in a reverse substitution /%s.\n", from->glyphs[k].name);
		} else {
			NEW(s);
			s->fromid = from->glyphs[k].index;
			s->toid = subtable->to->glyphs[k].index;
			s->fromname = from->glyphs[k].name;
			s->toname = subtable->to->glyphs[k].name;
			HASH_ADD_INT(h, fromid, s);
		}
	}
	HASH_SORT(h, by_from_id);
	if (HASH_COUNT(h) != from->numGlyphs || HASH_COUNT(h) != subtable->to->numGlyphs) {
		logWarning("[Consolidate] In this reverse subsitution lookup, some mappings are ignored.\n");
	}
	from->numGlyphs = HASH_COUNT(h);
	subtable->to->numGlyphs = HASH_COUNT(h);
	{
		gsub_single_map_hash *s, *tmp;
		glyphid_t j = 0;
		HASH_ITER(hh, h, s, tmp) {
			from->glyphs[j] = Handle.fromConsolidated(s->fromid, s->fromname);
			subtable->to->glyphs[j] = Handle.fromConsolidated(s->toid, s->toname);
			j++;
			HASH_DEL(h, s);
			FREE(s);
		}
	}
	return false;
}
