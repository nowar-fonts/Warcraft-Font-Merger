#include "gsub-ligature.h"

bool consolidate_gsub_ligature(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                               const otfcc_Options *options) {
	subtable_gsub_ligature *subtable = &(_subtable->gsub_ligature);
	subtable_gsub_ligature nt;
	iSubtable_gsub_ligature.init(&nt);
	for (glyphid_t k = 0; k < subtable->length; k++) {
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &subtable->items[k].to)) {
			logWarning("[Consolidate] Ignored missing glyph /%s.\n", subtable->items[k].to.name);
			continue;
		}
		fontop_consolidateCoverage(font, subtable->items[k].from, options);
		Coverage.shrink(subtable->items[k].from, false);
		if (!subtable->items[k].from->numGlyphs) {
			logWarning("[Consolidate] Ignoring empty ligature substitution to "
			           "glyph /%s.\n",
			           subtable->items[k].to.name);
			continue;
		}
		iSubtable_gsub_ligature.push(
		    &nt, ((otl_GsubLigatureEntry){
		             .from = subtable->items[k].from, .to = Handle.dup(subtable->items[k].to),
		         }));
		subtable->items[k].from = NULL;
	}
	iSubtable_gsub_ligature.replace(subtable, nt);
	return (subtable->length == 0);
}
