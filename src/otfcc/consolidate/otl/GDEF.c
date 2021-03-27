#include "GDEF.h"

typedef struct {
	int gid;
	sds name;
	otl_CaretValueList carets;
	UT_hash_handle hh;
} GDEF_ligcaret_hash;
static int by_gid(GDEF_ligcaret_hash *a, GDEF_ligcaret_hash *b) {
	return a->gid - b->gid;
}
void consolidate_GDEF(otfcc_Font *font, table_GDEF *gdef, const otfcc_Options *options) {
	if (!font || !font->glyph_order || !gdef) return;
	if (gdef->glyphClassDef) {
		fontop_consolidateClassDef(font, gdef->glyphClassDef, options);
		otl_iClassDef.shrink(gdef->glyphClassDef);
		if (!gdef->glyphClassDef->numGlyphs) {
			otl_iClassDef.free(gdef->glyphClassDef);
			gdef->glyphClassDef = NULL;
		}
	}
	if (gdef->markAttachClassDef) {
		fontop_consolidateClassDef(font, gdef->markAttachClassDef, options);
		otl_iClassDef.shrink(gdef->markAttachClassDef);
		if (!gdef->markAttachClassDef->numGlyphs) {
			otl_iClassDef.free(gdef->markAttachClassDef);
			gdef->markAttachClassDef = NULL;
		}
	}
	if (gdef->ligCarets.length) {
		GDEF_ligcaret_hash *h = NULL;
		for (glyphid_t j = 0; j < gdef->ligCarets.length; j++) {
			GDEF_ligcaret_hash *s;
			if (!GlyphOrder.consolidateHandle(font->glyph_order, &gdef->ligCarets.items[j].glyph)) {
				continue;
			}
			int gid = gdef->ligCarets.items[j].glyph.index;
			sds gname = sdsdup(gdef->ligCarets.items[j].glyph.name);
			if (gname) {
				HASH_FIND_INT(h, &gid, s);
				if (!s) {
					NEW(s);
					s->gid = gid;
					s->name = gname;
					otl_iCaretValueList.move(&s->carets, &gdef->ligCarets.items[j].carets);
					HASH_ADD_INT(h, gid, s);
				} else {
					logWarning("[Consolidate] Detected caret value double-mapping about glyph %s",
					           gname);
				}
			}
		}
		HASH_SORT(h, by_gid);
		otl_iLigCaretTable.clear(&gdef->ligCarets);
		GDEF_ligcaret_hash *s, *tmp;
		HASH_ITER(hh, h, s, tmp) {
			otl_CaretValueRecord v = {
			    .glyph = Handle.fromConsolidated(s->gid, s->name),
			};
			otl_iCaretValueList.move(&v.carets, &s->carets);
			otl_iLigCaretTable.push(&gdef->ligCarets, v);
			sdsfree(s->name);
			HASH_DEL(h, s);
			FREE(s);
		}
	}
}
