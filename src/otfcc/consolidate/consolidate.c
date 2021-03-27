#include "consolidate.h"

#include "support/util.h"
#include "table/all.h"

#include "otl/gsub-single.h"
#include "otl/gsub-multi.h"
#include "otl/gsub-ligature.h"
#include "otl/gsub-reverse.h"
#include "otl/gpos-single.h"
#include "otl/gpos-pair.h"
#include "otl/gpos-cursive.h"
#include "otl/chaining.h"
#include "otl/mark.h"
#include "otl/GDEF.h"

// Consolidation
// Replace name entries in json to ids and do some check
static int by_stem_pos(const glyf_PostscriptStemDef *a, const glyf_PostscriptStemDef *b) {
	if (a->position == b->position) {
		return (int)a->map - (int)b->map;
	} else if (a->position > b->position) {
		return 1;
	} else {
		return -1;
	}
}
static int by_mask_pointindex(const glyf_PostscriptHintMask *a, const glyf_PostscriptHintMask *b) {
	return a->contoursBefore == b->contoursBefore ? a->pointsBefore - b->pointsBefore
	                                              : a->contoursBefore - b->contoursBefore;
}

static void consolidateGlyphContours(glyf_Glyph *g, const otfcc_Options *options) {
	shapeid_t nContoursConsolidated = 0;
	shapeid_t skip = 0;
	for (shapeid_t j = 0; j < g->contours.length; j++) {
		if (g->contours.items[j].length) {
			g->contours.items[j - skip] = g->contours.items[j];
			nContoursConsolidated += 1;
		} else {
			glyf_iContourList.disposeItem(&g->contours, j);
			logWarning("[Consolidate] Removed empty contour #%d in glyph %s.\n", j, g->name);
			skip += 1;
		}
	}
	g->contours.length = nContoursConsolidated;
}
static void consolidateGlyphReferences(glyf_Glyph *g, otfcc_Font *font,
                                       const otfcc_Options *options) {
	shapeid_t nReferencesConsolidated = 0;
	shapeid_t skip = 0;
	for (shapeid_t j = 0; j < g->references.length; j++) {
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &g->references.items[j].glyph)) {
			logWarning("[Consolidate] Ignored absent glyph component reference /%s within /%s.\n",
			           g->references.items[j].glyph.name, g->name);
			glyf_iReferenceList.disposeItem(&(g->references), j);
			skip += 1;
		} else {
			g->references.items[j - skip] = g->references.items[j];
			nReferencesConsolidated += 1;
		}
	}
	g->references.length = nReferencesConsolidated;
}
static void consolidateGlyphHints(glyf_Glyph *g, const otfcc_Options *options) {
	// sort stems
	if (g->stemH.length) {
		for (shapeid_t j = 0; j < g->stemH.length; j++) {
			g->stemH.items[j].map = j;
		}
		glyf_iStemDefList.sort(&g->stemH, by_stem_pos);
	}
	if (g->stemV.length) {
		for (shapeid_t j = 0; j < g->stemV.length; j++) {
			g->stemV.items[j].map = j;
		}
		glyf_iStemDefList.sort(&g->stemV, by_stem_pos);
	}
	shapeid_t *hmap;
	NEW(hmap, g->stemH.length);
	shapeid_t *vmap;
	NEW(vmap, g->stemV.length);
	for (shapeid_t j = 0; j < g->stemH.length; j++) {
		hmap[g->stemH.items[j].map] = j;
	}
	for (shapeid_t j = 0; j < g->stemV.length; j++) {
		vmap[g->stemV.items[j].map] = j;
	}
	// sort masks
	if (g->hintMasks.length) {
		glyf_iMaskList.sort(&g->hintMasks, by_mask_pointindex);
		for (shapeid_t j = 0; j < g->hintMasks.length; j++) {
			glyf_PostscriptHintMask oldmask = g->hintMasks.items[j]; // copy
			for (shapeid_t k = 0; k < g->stemH.length; k++) {
				g->hintMasks.items[j].maskH[k] = oldmask.maskH[hmap[k]];
			}
			for (shapeid_t k = 0; k < g->stemV.length; k++) {
				g->hintMasks.items[j].maskV[k] = oldmask.maskV[vmap[k]];
			}
		}
	}
	if (g->contourMasks.length) {
		glyf_iMaskList.sort(&g->contourMasks, by_mask_pointindex);
		for (shapeid_t j = 0; j < g->contourMasks.length; j++) {
			glyf_PostscriptHintMask oldmask = g->contourMasks.items[j]; // copy
			for (shapeid_t k = 0; k < g->stemH.length; k++) {
				g->contourMasks.items[j].maskH[k] = oldmask.maskH[hmap[k]];
			}
			for (shapeid_t k = 0; k < g->stemV.length; k++) {
				g->contourMasks.items[j].maskV[k] = oldmask.maskV[vmap[k]];
			}
		}
	}
	FREE(hmap);
	FREE(vmap);
}
static void consolidateFDSelect(fd_handle *h, table_CFF *cff, const otfcc_Options *options,
                                const sds gname) {
	if (!cff || !cff->fdArray || !cff->fdArrayCount) return;
	// Consolidate fdSelect
	if (h->state == HANDLE_STATE_INDEX) {
		if (h->index >= cff->fdArrayCount) { h->index = 0; }
		Handle.consolidateTo(h, h->index, cff->fdArray[h->index]->fontName);
	} else if (h->name) {
		bool found = false;
		for (tableid_t j = 0; j < cff->fdArrayCount; j++) {
			if (strcmp(h->name, cff->fdArray[j]->fontName) == 0) {
				found = true;
				Handle.consolidateTo(h, j, cff->fdArray[j]->fontName);
				break;
			}
		}
		if (!found) {
			logWarning("[Consolidate] CID Subfont %s is not defined. (in glyph /%s).\n", h->name,
			           gname);
			Handle.dispose(h);
		}
	} else if (h->name) {
		Handle.dispose(h);
	}
}
void consolidateGlyph(glyf_Glyph *g, otfcc_Font *font, const otfcc_Options *options) {
	consolidateGlyphContours(g, options);
	consolidateGlyphReferences(g, font, options);
	consolidateGlyphHints(g, options);
	consolidateFDSelect(&g->fdSelect, font->CFF_, options, g->name);
}

// Anchored reference consolidation
bool consolidateAnchorRef(table_glyf *table, glyf_ComponentReference *gr,
                          glyf_ComponentReference *rr, const otfcc_Options *options);
bool getPointCoordinates(table_glyf *table, glyf_ComponentReference *gr, shapeid_t n,
                         shapeid_t *stated, VQ *x, VQ *y, const otfcc_Options *options) {
	glyphid_t j = gr->glyph.index;
	glyf_Glyph *g = table->items[j];
	for (shapeid_t c = 0; c < g->contours.length; c++) {
		for (shapeid_t pj = 0; pj < g->contours.items[c].length; pj++) {
			if (*stated == n) {
				glyf_Point *p = &(g->contours.items[c].items[pj]);
				iVQ.replace(x, iVQ.pointLinearTfm(gr->x, gr->a, p->x, gr->b, p->y));
				iVQ.replace(y, iVQ.pointLinearTfm(gr->y, gr->c, p->x, gr->d, p->y));
				return true;
			}
			*stated += 1;
		}
	}
	for (shapeid_t r = 0; r < g->references.length; r++) {
		glyf_ComponentReference *rr = &(g->references.items[r]);

		consolidateAnchorRef(table, gr, rr, options);

		// composite affine transformations
		glyf_ComponentReference ref = glyf_iComponentReference.empty();
		ref.glyph = Handle.fromIndex(g->references.items[r].glyph.index);
		ref.a = gr->a * rr->a + rr->b * gr->c;
		ref.b = rr->a * gr->b + rr->b * gr->d;
		ref.c = gr->a * rr->c + gr->c * rr->d;
		ref.d = gr->b * rr->c + rr->d * gr->d;
		iVQ.replace(&ref.x, iVQ.pointLinearTfm(rr->x, rr->a, gr->x, rr->b, gr->y));
		iVQ.replace(&ref.y, iVQ.pointLinearTfm(rr->y, rr->c, gr->x, rr->d, gr->y));

		bool success = getPointCoordinates(table, &ref, n, stated, x, y, options);
		glyf_iComponentReference.dispose(&ref);
		if (success) return true;
	}
	return false;
}

bool consolidateAnchorRef(table_glyf *table, glyf_ComponentReference *gr,
                          glyf_ComponentReference *rr, const otfcc_Options *options) {
	if (rr->isAnchored == REF_ANCHOR_CONSOLIDATED || rr->isAnchored == REF_XY) return true;
	if (rr->isAnchored == REF_ANCHOR_CONSOLIDATING_ANCHOR ||
	    rr->isAnchored == REF_ANCHOR_CONSOLIDATING_XY) {
		logWarning(
		    "Found circular reference of out-of-range point reference in anchored reference.");
		rr->isAnchored = REF_XY;
		return false;
	}
	if (rr->isAnchored == REF_ANCHOR_ANCHOR) {
		rr->isAnchored = REF_ANCHOR_CONSOLIDATING_ANCHOR;
	} else {
		rr->isAnchored = REF_ANCHOR_CONSOLIDATING_XY;
	}
	VQ innerX = iVQ.neutral(), outerX = iVQ.neutral();
	VQ innerY = iVQ.neutral(), outerY = iVQ.neutral();
	shapeid_t innerCounter = 0, outerCounter = 0;

	glyf_ComponentReference rr1 = glyf_iComponentReference.empty();
	rr1.glyph = Handle.fromIndex(rr->glyph.index);

	bool s1 = getPointCoordinates(table, gr, rr->outer, &outerCounter, &outerX, &outerY, options);
	bool s2 = getPointCoordinates(table, &rr1, rr->inner, &innerCounter, &innerX, &innerY, options);
	if (!s1) { logWarning("Failed to access point %d in outer glyph.", rr->outer); }
	if (!s2) {
		logWarning("Failed to access point %d in reference to %s.", rr->outer, rr->glyph.name);
	}

	VQ rrx = iVQ.pointLinearTfm(outerX, -rr->a, innerX, -rr->b, innerY);
	VQ rry = iVQ.pointLinearTfm(outerY, -rr->c, innerX, -rr->d, innerY);

	if (rr->isAnchored == REF_ANCHOR_CONSOLIDATING_ANCHOR) {
		iVQ.replace(&rr->x, rrx);
		iVQ.replace(&rr->y, rry);
		rr->isAnchored = REF_ANCHOR_CONSOLIDATED;
	} else {
		if (fabs(iVQ.getStill(rr->x) - iVQ.getStill(rrx)) > 0.5 &&
		    fabs(iVQ.getStill(rr->y) - iVQ.getStill(rry)) > 0.5) {
			logWarning("Anchored reference to %s does not match its X/Y offset data.",
			           rr->glyph.name);
		}
		rr->isAnchored = REF_ANCHOR_CONSOLIDATED;
		iVQ.dispose(&rrx), iVQ.dispose(&rry);
	}
	glyf_iComponentReference.dispose(&rr1);
	iVQ.dispose(&innerX), iVQ.dispose(&innerY);
	iVQ.dispose(&outerX), iVQ.dispose(&outerY);
	return false;
}

void consolidateGlyf(otfcc_Font *font, const otfcc_Options *options) {
	if (!font->glyph_order || !font->glyf) return;
	for (glyphid_t j = 0; j < font->glyf->length; j++) {
		if (font->glyf->items[j]) {
			consolidateGlyph(font->glyf->items[j], font, options);
		} else {
			font->glyf->items[j] = otfcc_newGlyf_glyph();
		}
	}
	for (glyphid_t j = 0; j < font->glyf->length; j++) {
		glyf_Glyph *g = font->glyf->items[j];
		loggedStep("%s", g->name) {
			glyf_ComponentReference gr = glyf_iComponentReference.empty();
			gr.glyph = Handle.fromIndex(j);

			for (shapeid_t r = 0; r < g->references.length; r++) {
				glyf_ComponentReference *rr = &(g->references.items[r]);
				consolidateAnchorRef(font->glyf, &gr, rr, options);
			}
			glyf_iComponentReference.dispose(&gr);
		}
	}
}

void consolidateCmap(otfcc_Font *font, const otfcc_Options *options) {
	if (font->glyph_order && font->cmap) {
		cmap_Entry *item;
		foreach_hash(item, font->cmap->unicodes) {
			if (!GlyphOrder.consolidateHandle(font->glyph_order, &item->glyph)) {
				logWarning("[Consolidate] Ignored mapping U+%04X to non-existent glyph /%s.\n",
				           item->unicode, item->glyph.name);
				Handle.dispose(&item->glyph);
			}
		}
	}
	if (font->glyph_order && font->cmap) {
		cmap_UVS_Entry *item;
		foreach_hash(item, font->cmap->uvs) {
			if (!GlyphOrder.consolidateHandle(font->glyph_order, &item->glyph)) {
				logWarning("[Consolidate] Ignored UVS mapping [U+%04X U+%04X] to non-existent "
				           "glyph /%s.\n",
				           item->key.unicode, item->key.selector, item->glyph.name);
				Handle.dispose(&item->glyph);
			}
		}
	}
}

typedef bool (*otl_consolidation_function)(otfcc_Font *, table_OTL *, otl_Subtable *,
                                           const otfcc_Options *);
typedef void (*subtable_remover)(otl_Subtable *);
#define LOOKUP_CONSOLIDATOR(llt, fn, fndel)                                                        \
	__declare_otl_consolidation(llt, fn, (subtable_remover)fndel, font, table, lookup, options);

static void __declare_otl_consolidation(otl_LookupType type, otl_consolidation_function fn,
                                        subtable_remover fndel, otfcc_Font *font, table_OTL *table,
                                        otl_Lookup *lookup, const otfcc_Options *options) {
	if (!lookup || !lookup->subtables.length || lookup->type != type) return;
	loggedStep("%s", lookup->name) {
		for (tableid_t j = 0; j < lookup->subtables.length; j++) {
			if (!lookup->subtables.items[j]) {
				logWarning("[Consolidate] Ignored empty subtable %d of lookup %s.\n", j,
				           lookup->name);
				continue;
			}
			bool subtableRemoved;
			// loggedStep("Subtable %d", j) {
			subtableRemoved = fn(font, table, lookup->subtables.items[j], options);
			//}
			if (subtableRemoved) {
				fndel(lookup->subtables.items[j]);
				lookup->subtables.items[j] = NULL;
				logWarning("[Consolidate] Ignored empty subtable %d of lookup %s.\n", j,
				           lookup->name);
			}
		}
		tableid_t k = 0;
		for (tableid_t j = 0; j < lookup->subtables.length; j++) {
			if (lookup->subtables.items[j]) {
				lookup->subtables.items[k++] = lookup->subtables.items[j];
			}
		}
		lookup->subtables.length = k;
		if (!k) {
			logWarning("[Consolidate] Lookup %s is empty and will be removed.\n", lookup->name);
		}
	}
}

void otfcc_consolidate_lookup(otfcc_Font *font, table_OTL *table, otl_Lookup *lookup,
                              const otfcc_Options *options) {
	LOOKUP_CONSOLIDATOR(otl_type_gsub_single, consolidate_gsub_single, iSubtable_gsub_single.free);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_multiple, consolidate_gsub_multi, iSubtable_gsub_multi.free);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_alternate, consolidate_gsub_alternative,
	                    iSubtable_gsub_multi.free);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_ligature, consolidate_gsub_ligature,
	                    iSubtable_gsub_ligature.free);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_chaining, consolidate_chaining, iSubtable_chaining.free);
	LOOKUP_CONSOLIDATOR(otl_type_gsub_reverse, consolidate_gsub_reverse,
	                    iSubtable_gsub_reverse.free);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_single, consolidate_gpos_single, iSubtable_gpos_single.free);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_pair, consolidate_gpos_pair, iSubtable_gpos_pair.free);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_cursive, consolidate_gpos_cursive,
	                    iSubtable_gpos_cursive.free);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_chaining, consolidate_chaining, iSubtable_chaining.free);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_markToBase, consolidate_mark_to_single,
	                    iSubtable_gpos_markToSingle.free);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_markToMark, consolidate_mark_to_single,
	                    iSubtable_gpos_markToSingle.free);
	LOOKUP_CONSOLIDATOR(otl_type_gpos_markToLigature, consolidate_mark_to_ligature,
	                    iSubtable_gpos_markToLigature.free);
}

static bool lookupRefIsNotEmpty(const otl_LookupRef *rLut, void *env) {
	return rLut && *rLut && (*rLut)->subtables.length > 0;
}
static bool featureRefIsNotEmpty(const otl_FeatureRef *rFeat, void *env) {
	return rFeat && *rFeat && (*rFeat)->lookups.length > 0;
}
static bool lookupIsNotEmpty(const otl_LookupPtr *rLut, void *env) {
	return rLut && *rLut && (*rLut)->subtables.length > 0;
}
static bool featureIsNotEmpty(const otl_FeaturePtr *rFeat, void *env) {
	return rFeat && *rFeat && (*rFeat)->lookups.length > 0;
}

static void consolidateOTLTable(otfcc_Font *font, table_OTL *table, const otfcc_Options *options) {
	if (!font->glyph_order || !table) return;
	do {
		tableid_t featN = table->features.length;
		tableid_t lutN = table->lookups.length;

		// Perform consolidation
		for (tableid_t j = 0; j < table->lookups.length; j++) {
			otfcc_consolidate_lookup(font, table, table->lookups.items[j], options);
		}
		// remove empty features
		for (tableid_t j = 0; j < table->features.length; j++) {
			otl_Feature *feature = table->features.items[j];
			otl_iLookupRefList.filterEnv(&feature->lookups, lookupRefIsNotEmpty, NULL);
		}
		// remove empty lookups
		for (tableid_t j = 0; j < table->languages.length; j++) {
			otl_LanguageSystem *lang = table->languages.items[j];
			otl_iFeatureRefList.filterEnv(&lang->features, featureRefIsNotEmpty, NULL);
		}
		otl_iLookupList.filterEnv(&table->lookups, lookupIsNotEmpty, NULL);
		otl_iFeatureList.filterEnv(&table->features, featureIsNotEmpty, NULL);

		tableid_t featN1 = table->features.length;
		tableid_t lutN1 = table->lookups.length;
		if (featN1 >= featN && lutN1 >= lutN) break;
	} while (true);
}

static void consolidateOTL(otfcc_Font *font, const otfcc_Options *options) {
	loggedStep("GSUB") {
		consolidateOTLTable(font, font->GSUB, options);
	}
	loggedStep("GPOS") {
		consolidateOTLTable(font, font->GPOS, options);
	}
	loggedStep("GDEF") {
		consolidate_GDEF(font, font->GDEF, options);
	}
}

static void consolidateCOLR(otfcc_Font *font, const otfcc_Options *options) {
	if (!font || !font->COLR || !font->glyph_order) return;
	table_COLR *consolidated = table_iCOLR.create();
	foreach (colr_Mapping *mapping, *(font->COLR)) {
		if (!GlyphOrder.consolidateHandle(font->glyph_order, &mapping->glyph)) {
			logWarning("[Consolidate] Ignored missing glyph of /%s", mapping->glyph.name);
			continue;
		}
		colr_Mapping m;
		Handle.copy(&m.glyph, &mapping->glyph);
		colr_iLayerList.init(&m.layers);
		foreach (colr_Layer *layer, mapping->layers) {
			if (!GlyphOrder.consolidateHandle(font->glyph_order, &layer->glyph)) {
				logWarning("[Consolidate] Ignored missing glyph of /%s", layer->glyph.name);
				continue;
			}
			colr_Layer layer1;
			colr_iLayer.copy(&layer1, layer);
			colr_iLayerList.push(&m.layers, layer1);
		}
		if (mapping->layers.length) {
			table_iCOLR.push(consolidated, m);
		} else {
			logWarning("[Consolidate] COLR decomposition for /%s is empth", mapping->glyph.name);
			colr_iMapping.dispose(&m);
		}
	}
	table_iCOLR.free(font->COLR);
	font->COLR = consolidated;
}

static int compareTSIEntry(const tsi_Entry *a, const tsi_Entry *b) {
	if (a->type != b->type) return a->type - b->type;
	return a->glyph.index - b->glyph.index;
}

static void consolidateTSI(otfcc_Font *font, table_TSI **_tsi, const otfcc_Options *options) {
	table_TSI *tsi = *_tsi;
	if (!font || !font->glyf || !tsi || !font->glyph_order) return;
	table_TSI *consolidated = table_iTSI.create();
	sds *gidEntries;
	NEW_CLEAN_N(gidEntries, font->glyf->length);

	foreach (tsi_Entry *entry, *tsi) {
		if (entry->type == TSI_GLYPH) {
			if (GlyphOrder.consolidateHandle(font->glyph_order, &entry->glyph)) {
				if (gidEntries[entry->glyph.index]) sdsfree(gidEntries[entry->glyph.index]);
				gidEntries[entry->glyph.index] = entry->content;
				entry->content = NULL;
			} else {
				logWarning("[Consolidate] Ignored missing glyph of /%s", entry->glyph.name);
			}
		} else {
			tsi_Entry e;
			tsi_iEntry.copy(&e, entry);
			table_iTSI.push(consolidated, e);
		}
	}
	for (glyphid_t j = 0; j < font->glyf->length; j++) {
		tsi_Entry e;
		e.type = TSI_GLYPH;
		e.glyph = Handle.fromIndex(j);
		GlyphOrder.consolidateHandle(font->glyph_order, &e.glyph);
		e.content = gidEntries[j] ? gidEntries[j] : sdsempty();
		table_iTSI.push(consolidated, e);
	}
	table_iTSI.free(tsi);
	FREE(gidEntries);
	table_iTSI.sort(consolidated, compareTSIEntry);
	*_tsi = consolidated;
}

void otfcc_consolidateFont(otfcc_Font *font, const otfcc_Options *options) {
	// In case we don’t have a glyph order, make one.
	if (font->glyf && !font->glyph_order) {
		otfcc_GlyphOrder *go = GlyphOrder.create();
		for (glyphid_t j = 0; j < font->glyf->length; j++) {
			sds name;
			sds glyfName = font->glyf->items[j]->name;
			if (glyfName) {
				name = sdsdup(glyfName);
			} else {
				name = sdscatprintf(sdsempty(), "$$gid%d", j);
				font->glyf->items[j]->name = sdsdup(name);
			}
			if (!GlyphOrder.setByName(go, name, j)) {
				logWarning("[Consolidate] Glyph name %s is already in use.", name);
				uint32_t suffix = 2;
				bool success = false;
				do {
					sds newname = sdscatfmt(sdsempty(), "%s_%u", name, suffix);
					success = GlyphOrder.setByName(go, newname, j);
					if (!success) {
						sdsfree(newname);
						suffix += 1;
					} else {
						logWarning("[Consolidate] Glyph %s is renamed into %s.", name, newname);
						sdsfree(font->glyf->items[j]->name);
						font->glyf->items[j]->name = sdsdup(newname);
					}
				} while (!success);
				sdsfree(name);
			}
		}
		font->glyph_order = go;
	}
	loggedStep("glyf") {
		consolidateGlyf(font, options);
	}
	loggedStep("cmap") {
		consolidateCmap(font, options);
	}
	if (font->glyf) consolidateOTL(font, options);
	loggedStep("COLR") {
		consolidateCOLR(font, options);
	}
	loggedStep("TSI_01") {
		consolidateTSI(font, &font->TSI_01, options);
	}
	loggedStep("TSI_23") {
		consolidateTSI(font, &font->TSI_23, options);
	}
	loggedStep("TSI5") {
		fontop_consolidateClassDef(font, font->TSI5, options);
	}
}
