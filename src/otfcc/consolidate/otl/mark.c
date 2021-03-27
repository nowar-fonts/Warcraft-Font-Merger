#include "mark.h"

typedef struct {
	int gid;
	sds name;
	glyphclass_t markClass;
	otl_Anchor anchor;
	UT_hash_handle hh;
} mark_hash;
static int mark_by_gid(mark_hash *a, mark_hash *b) {
	return a->gid - b->gid;
}
typedef struct {
	int gid;
	sds name;
	otl_Anchor *anchors;
	UT_hash_handle hh;
} base_hash;
static int base_by_gid(base_hash *a, base_hash *b) {
	return a->gid - b->gid;
}
typedef struct {
	int gid;
	sds name;
	glyphid_t componentCount;
	otl_Anchor **anchors;
	UT_hash_handle hh;
} lig_hash;
static int lig_by_gid(lig_hash *a, lig_hash *b) {
	return a->gid - b->gid;
}

static void consolidateMarkArray(otfcc_Font *font, table_OTL *table, const otfcc_Options *options,
                                 otl_MarkArray *markArray, glyphclass_t classCount) {
	mark_hash *hm = NULL;
	for (glyphid_t k = 0; k < markArray->length; k++) {
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &markArray->items[k].glyph)) {
			logWarning("[Consolidate] Ignored unknown glyph name %s.",
			           markArray->items[k].glyph.name);
			continue;
		}
		mark_hash *s = NULL;
		int gid = markArray->items[k].glyph.index;
		HASH_FIND_INT(hm, &gid, s);
		if (!s && markArray->items[k].anchor.present &&
		    markArray->items[k].markClass < classCount) {
			NEW(s);
			s->gid = markArray->items[k].glyph.index;
			s->name = sdsdup(markArray->items[k].glyph.name);
			s->markClass = markArray->items[k].markClass;
			s->anchor = markArray->items[k].anchor;
			HASH_ADD_INT(hm, gid, s);
		} else {
			logWarning("[Consolidate] Ignored invalid or double-mapping mark definition for /%s.",
			           markArray->items[k].glyph.name);
		}
	}
	HASH_SORT(hm, mark_by_gid);
	otl_iMarkArray.clear(markArray);
	mark_hash *s, *tmp;
	HASH_ITER(hh, hm, s, tmp) {
		otl_iMarkArray.push(markArray, ((otl_MarkRecord){
		                                   .glyph = Handle.fromConsolidated(s->gid, s->name),
		                                   .markClass = s->markClass,
		                                   .anchor = s->anchor,
		                               }));
		sdsfree(s->name);
		HASH_DEL(hm, s);
		FREE(s);
	}
}

static void consolidateBaseArray(otfcc_Font *font, table_OTL *table, const otfcc_Options *options,
                                 otl_BaseArray *baseArray) {
	// consolidate bases
	base_hash *hm = NULL;
	for (glyphid_t k = 0; k < baseArray->length; k++) {
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &baseArray->items[k].glyph)) {
			logWarning("[Consolidate] Ignored unknown glyph name %s.",
			           baseArray->items[k].glyph.name);
			continue;
		}
		base_hash *s = NULL;
		int gid = baseArray->items[k].glyph.index;
		HASH_FIND_INT(hm, &gid, s);
		if (!s) {
			NEW(s);
			s->gid = baseArray->items[k].glyph.index;
			s->name = sdsdup(baseArray->items[k].glyph.name);
			s->anchors = baseArray->items[k].anchors;
			baseArray->items[k].anchors = NULL; // Transfer ownership
			HASH_ADD_INT(hm, gid, s);
		} else {
			logWarning("[Consolidate] Ignored anchor double-definition for /%s.",
			           baseArray->items[k].glyph.name);
		}
	}
	HASH_SORT(hm, base_by_gid);
	otl_iBaseArray.clear(baseArray);
	base_hash *s, *tmp;
	HASH_ITER(hh, hm, s, tmp) {
		otl_iBaseArray.push(
		    baseArray, ((otl_BaseRecord){
		                   .glyph = Handle.fromConsolidated(s->gid, s->name), .anchors = s->anchors,
		               }));
		sdsfree(s->name);
		HASH_DEL(hm, s);
		FREE(s);
	}
}

static void consolidateLigArray(otfcc_Font *font, table_OTL *table, const otfcc_Options *options,
                                otl_LigatureArray *ligArray) {
	lig_hash *hm = NULL;
	for (glyphid_t k = 0; k < ligArray->length; k++) {
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &ligArray->items[k].glyph)) {
			logWarning("[Consolidate] Ignored unknown glyph name %s.",
			           ligArray->items[k].glyph.name);
			continue;
		}
		lig_hash *s = NULL;
		int gid = ligArray->items[k].glyph.index;
		HASH_FIND_INT(hm, &gid, s);
		if (!s) {
			NEW(s);
			s->gid = ligArray->items[k].glyph.index;
			s->name = sdsdup(ligArray->items[k].glyph.name);
			s->componentCount = ligArray->items[k].componentCount;
			s->anchors = ligArray->items[k].anchors;
			ligArray->items[k].anchors = NULL;
			HASH_ADD_INT(hm, gid, s);
		} else {
			logWarning("[Consolidate] Ignored anchor double-definition for /%s.",
			           ligArray->items[k].glyph.name);
		}
	}
	HASH_SORT(hm, lig_by_gid);
	otl_iLigatureArray.clear(ligArray);
	lig_hash *s, *tmp;
	HASH_ITER(hh, hm, s, tmp) {
		otl_iLigatureArray.push(ligArray, ((otl_LigatureBaseRecord){
		                                      .glyph = Handle.fromConsolidated(s->gid, s->name),
		                                      .componentCount = s->componentCount,
		                                      .anchors = s->anchors,
		                                  }));
		sdsfree(s->name);
		HASH_DEL(hm, s);
		FREE(s);
	}
}

bool consolidate_mark_to_single(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                                const otfcc_Options *options) {
	subtable_gpos_markToSingle *subtable = &(_subtable->gpos_markToSingle);
	consolidateMarkArray(font, table, options, &subtable->markArray, subtable->classCount);
	consolidateBaseArray(font, table, options, &subtable->baseArray);
	return (subtable->markArray.length == 0) || (subtable->baseArray.length == 0);
}

bool consolidate_mark_to_ligature(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                                  const otfcc_Options *options) {
	subtable_gpos_markToLigature *subtable = &(_subtable->gpos_markToLigature);
	consolidateMarkArray(font, table, options, &subtable->markArray, subtable->classCount);
	consolidateLigArray(font, table, options, &subtable->ligArray);
	return (subtable->markArray.length == 0) || (subtable->ligArray.length == 0);
}
