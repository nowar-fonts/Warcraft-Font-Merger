#include "../glyf.h"

#include "support/util.h"
#include "support/ttinstr/ttinstr.h"

// point
static void createPoint(glyf_Point *p) {
	p->x = iVQ.createStill(0);
	p->y = iVQ.createStill(0);
	p->onCurve = true;
}
static void copyPoint(glyf_Point *dst, const glyf_Point *src) {
	iVQ.copy(&dst->x, &src->x);
	iVQ.copy(&dst->y, &src->y);
	dst->onCurve = src->onCurve;
}
static void disposePoint(glyf_Point *p) {
	iVQ.dispose(&p->x);
	iVQ.dispose(&p->y);
}

caryll_standardValType(glyf_Point, glyf_iPoint, createPoint, copyPoint, disposePoint);

// contour
caryll_standardVectorImpl(glyf_Contour, glyf_Point, glyf_iPoint, glyf_iContour);
caryll_standardVectorImpl(glyf_ContourList, glyf_Contour, glyf_iContour, glyf_iContourList);

// ref
static INLINE void initGlyfReference(glyf_ComponentReference *ref) {
	ref->glyph = Handle.empty();
	ref->x = iVQ.createStill(0);
	ref->y = iVQ.createStill(0);
	ref->a = 1;
	ref->b = 0;
	ref->c = 0;
	ref->d = 1;
	ref->isAnchored = REF_XY;
	ref->inner = ref->outer = 0;
	ref->roundToGrid = false;
	ref->useMyMetrics = false;
}
static void copyGlyfReference(glyf_ComponentReference *dst, const glyf_ComponentReference *src) {
	iVQ.copy(&dst->x, &src->x);
	iVQ.copy(&dst->y, &src->y);
	Handle.copy(&dst->glyph, &src->glyph);
	dst->a = src->a;
	dst->b = src->b;
	dst->c = src->c;
	dst->d = src->d;
	dst->isAnchored = src->isAnchored;
	dst->inner = src->inner;
	dst->outer = src->outer;
	dst->roundToGrid = src->roundToGrid;
	dst->useMyMetrics = src->useMyMetrics;
}
static INLINE void disposeGlyfReference(glyf_ComponentReference *ref) {
	iVQ.dispose(&ref->x);
	iVQ.dispose(&ref->y);
	Handle.dispose(&ref->glyph);
}
caryll_standardValType(glyf_ComponentReference, glyf_iComponentReference, initGlyfReference,
                       copyGlyfReference, disposeGlyfReference);
caryll_standardVectorImpl(glyf_ReferenceList, glyf_ComponentReference, glyf_iComponentReference,
                          glyf_iReferenceList);

// stem
caryll_standardType(glyf_PostscriptStemDef, glyf_iPostscriptStemDef);
caryll_standardVectorImpl(glyf_StemDefList, glyf_PostscriptStemDef, glyf_iPostscriptStemDef,
                          glyf_iStemDefList);

// mask
caryll_standardType(glyf_PostscriptHintMask, glyf_iPostscriptHintMask);
caryll_standardVectorImpl(glyf_MaskList, glyf_PostscriptHintMask, glyf_iPostscriptHintMask,
                          glyf_iMaskList);

glyf_Glyph *otfcc_newGlyf_glyph() {
	glyf_Glyph *g;
	NEW(g);
	g->name = NULL;
	iVQ.init(&g->horizontalOrigin);
	iVQ.init(&g->advanceWidth);
	iVQ.init(&g->verticalOrigin);
	iVQ.init(&g->advanceHeight);

	glyf_iContourList.init(&g->contours);
	glyf_iReferenceList.init(&g->references);
	glyf_iStemDefList.init(&g->stemH);
	glyf_iStemDefList.init(&g->stemV);
	glyf_iMaskList.init(&g->hintMasks);
	glyf_iMaskList.init(&g->contourMasks);

	g->instructionsLength = 0;
	g->instructions = NULL;
	g->fdSelect = Handle.empty();
	g->yPel = 0;

	g->stat.xMin = 0;
	g->stat.xMax = 0;
	g->stat.yMin = 0;
	g->stat.yMax = 0;
	g->stat.nestDepth = 0;
	g->stat.nPoints = 0;
	g->stat.nContours = 0;
	g->stat.nCompositePoints = 0;
	g->stat.nCompositeContours = 0;
	return g;
}
static void otfcc_deleteGlyf_glyph(glyf_Glyph *g) {
	if (!g) return;
	iVQ.dispose(&g->horizontalOrigin);
	iVQ.dispose(&g->advanceWidth);
	iVQ.dispose(&g->verticalOrigin);
	iVQ.dispose(&g->advanceHeight);
	sdsfree(g->name);
	glyf_iContourList.dispose(&g->contours);
	glyf_iReferenceList.dispose(&g->references);
	glyf_iStemDefList.dispose(&g->stemH);
	glyf_iStemDefList.dispose(&g->stemV);
	glyf_iMaskList.dispose(&g->hintMasks);
	glyf_iMaskList.dispose(&g->contourMasks);
	if (g->instructions) { FREE(g->instructions); }
	Handle.dispose(&g->fdSelect);
	g->name = NULL;
	FREE(g);
}

static INLINE void initGlyfPtr(glyf_GlyphPtr *g) {
	*g = NULL;
}
static void copyGlyfPtr(glyf_GlyphPtr *dst, const glyf_GlyphPtr *src) {
	*dst = *src;
}
static INLINE void disposeGlyfPtr(glyf_GlyphPtr *g) {
	otfcc_deleteGlyf_glyph(*g);
}
caryll_ElementInterfaceOf(glyf_GlyphPtr) glyf_iGlyphPtr = {
    .init = initGlyfPtr,
    .copy = copyGlyfPtr,
    .dispose = disposeGlyfPtr,
};
caryll_standardVectorImpl(table_glyf, glyf_GlyphPtr, glyf_iGlyphPtr, table_iGlyf);

// to json
static void glyf_glyph_dump_contours(glyf_Glyph *g, json_value *target, const GlyfIOContext *ctx) {
	if (!g->contours.length) return;
	json_value *contours = json_array_new(g->contours.length);
	for (shapeid_t k = 0; k < g->contours.length; k++) {
		glyf_Contour *c = &(g->contours.items[k]);
		json_value *contour = json_array_new(c->length);
		for (shapeid_t m = 0; m < c->length; m++) {
			json_value *point = json_object_new(4);
			json_object_push(point, "x", json_new_VQ(c->items[m].x, ctx->fvar));
			json_object_push(point, "y", json_new_VQ(c->items[m].y, ctx->fvar));
			json_object_push(point, "on", json_boolean_new(c->items[m].onCurve & MASK_ON_CURVE));
			json_array_push(contour, point);
		}
		json_array_push(contours, preserialize(contour));
	}
	json_object_push(target, "contours", contours);
}

static void glyf_glyph_dump_references(glyf_Glyph *g, json_value *target,
                                       const GlyfIOContext *ctx) {
	if (!g->references.length) return;
	json_value *references = json_array_new(g->references.length);
	for (shapeid_t k = 0; k < g->references.length; k++) {
		glyf_ComponentReference *r = &(g->references.items[k]);
		json_value *ref = json_object_new(9);
		json_object_push(ref, "glyph",
		                 json_string_new_length((uint32_t)sdslen(r->glyph.name), r->glyph.name));
		json_object_push(ref, "x", json_new_VQ(r->x, ctx->fvar));
		json_object_push(ref, "y", json_new_VQ(r->y, ctx->fvar));
		json_object_push(ref, "a", json_new_position(r->a));
		json_object_push(ref, "b", json_new_position(r->b));
		json_object_push(ref, "c", json_new_position(r->c));
		json_object_push(ref, "d", json_new_position(r->d));
		if (r->isAnchored != REF_XY) {
			json_object_push(ref, "isAnchored", json_boolean_new(true));
			json_object_push(ref, "inner", json_integer_new(r->inner));
			json_object_push(ref, "outer", json_integer_new(r->outer));
		}
		if (r->roundToGrid) { json_object_push(ref, "roundToGrid", json_boolean_new(true)); }
		if (r->useMyMetrics) { json_object_push(ref, "useMyMetrics", json_boolean_new(true)); }
		json_array_push(references, preserialize(ref));
	}
	json_object_push(target, "references", references);
}
static json_value *glyf_glyph_dump_stemdefs(glyf_StemDefList *stems) {
	json_value *a = json_array_new(stems->length);
	for (shapeid_t j = 0; j < stems->length; j++) {
		json_value *stem = json_object_new(3);
		json_object_push(stem, "position", json_new_position(stems->items[j].position));
		json_object_push(stem, "width", json_new_position(stems->items[j].width));
		json_array_push(a, stem);
	}
	return a;
}
static json_value *glyf_glyph_dump_maskdefs(glyf_MaskList *masks, glyf_StemDefList *hh,
                                            glyf_StemDefList *vv) {
	json_value *a = json_array_new(masks->length);
	for (shapeid_t j = 0; j < masks->length; j++) {
		json_value *mask = json_object_new(3);
		json_object_push(mask, "contoursBefore", json_integer_new(masks->items[j].contoursBefore));
		json_object_push(mask, "pointsBefore", json_integer_new(masks->items[j].pointsBefore));
		json_value *h = json_array_new(hh->length);
		for (shapeid_t k = 0; k < hh->length; k++) {
			json_array_push(h, json_boolean_new(masks->items[j].maskH[k]));
		}
		json_object_push(mask, "maskH", h);
		json_value *v = json_array_new(vv->length);
		for (shapeid_t k = 0; k < vv->length; k++) {
			json_array_push(v, json_boolean_new(masks->items[j].maskV[k]));
		}
		json_object_push(mask, "maskV", v);
		json_array_push(a, mask);
	}
	return a;
}

static json_value *glyf_dump_glyph(glyf_Glyph *g, const otfcc_Options *options,
                                   const GlyfIOContext *ctx) {
	json_value *glyph = json_object_new(12);
	json_object_push(glyph, "advanceWidth", json_new_VQ(g->advanceWidth, ctx->fvar));
	if (iVQ.isStill(g->horizontalOrigin) &&
	    fabs(iVQ.getStill(g->horizontalOrigin)) > 1.0 / 1000.0) {
		json_object_push(glyph, "horizontalOrigin", json_new_VQ(g->horizontalOrigin, ctx->fvar));
	}
	if (ctx->hasVerticalMetrics) {
		json_object_push(glyph, "advanceHeight", json_new_VQ(g->advanceHeight, ctx->fvar));
		json_object_push(glyph, "verticalOrigin", json_new_VQ(g->verticalOrigin, ctx->fvar));
	}
	glyf_glyph_dump_contours(g, glyph, ctx);
	glyf_glyph_dump_references(g, glyph, ctx);
	if (ctx->exportFDSelect) {
		json_object_push(glyph, "CFF_fdSelect", json_string_new(g->fdSelect.name));
		json_object_push(glyph, "CFF_CID", json_integer_new(g->cid));
	}

	// hinting data
	if (!options->ignore_hints) {
		if (g->instructions && g->instructionsLength) {
			json_object_push(glyph, "instructions",
			                 dump_ttinstr(g->instructions, g->instructionsLength, options));
		}
		if (g->stemH.length) {
			json_object_push(glyph, "stemH", preserialize(glyf_glyph_dump_stemdefs(&g->stemH)));
		}
		if (g->stemV.length) {
			json_object_push(glyph, "stemV", preserialize(glyf_glyph_dump_stemdefs(&g->stemV)));
		}
		if (g->hintMasks.length) {
			json_object_push(
			    glyph, "hintMasks",
			    preserialize(glyf_glyph_dump_maskdefs(&g->hintMasks, &g->stemH, &g->stemV)));
		}
		if (g->contourMasks.length) {
			json_object_push(
			    glyph, "contourMasks",
			    preserialize(glyf_glyph_dump_maskdefs(&g->contourMasks, &g->stemH, &g->stemV)));
		}
		if (g->yPel) { json_object_push(glyph, "LTSH_yPel", json_integer_new(g->yPel)); }
	}
	return glyph;
}
void otfcc_dump_glyphorder(const table_glyf *table, json_value *root) {
	if (!table) return;
	json_value *order = json_array_new(table->length);
	for (glyphid_t j = 0; j < table->length; j++) {
		json_array_push(order, json_string_new_length((uint32_t)sdslen(table->items[j]->name),
		                                              table->items[j]->name));
	}
	json_object_push(root, "glyph_order", preserialize(order));
}
void otfcc_dumpGlyf(const table_glyf *table, json_value *root, const otfcc_Options *options,
                    const GlyfIOContext *ctx) {
	if (!table) return;
	loggedStep("glyf") {
		json_value *glyf = json_object_new(table->length);
		for (glyphid_t j = 0; j < table->length; j++) {
			glyf_Glyph *g = table->items[j];
			json_object_push(glyf, g->name, glyf_dump_glyph(g, options, ctx));
		}
		json_object_push(root, "glyf", glyf);
		if (!options->ignore_glyph_order) otfcc_dump_glyphorder(table, root);
	}
}

// from json
static glyf_Point glyf_parse_point(json_value *pointdump) {
	glyf_Point point;
	glyf_iPoint.init(&point);
	if (!pointdump || pointdump->type != json_object) return point;
	for (uint32_t _k = 0; _k < pointdump->u.object.length; _k++) {
		char *ck = pointdump->u.object.values[_k].name;
		json_value *cv = pointdump->u.object.values[_k].value;
		if (strcmp(ck, "x") == 0) {
			iVQ.replace(&point.x, json_vqOf(cv, NULL));
		} else if (strcmp(ck, "y") == 0) {
			iVQ.replace(&point.y, json_vqOf(cv, NULL));
		} else if (strcmp(ck, "on") == 0) {
			point.onCurve = json_boolof(cv);
		}
	}
	return point;
}

static void glyf_parse_contours(json_value *col, glyf_Glyph *g) {
	if (!col) { return; }
	shapeid_t nContours = col->u.array.length;
	for (shapeid_t j = 0; j < nContours; j++) {
		json_value *contourdump = col->u.array.values[j];
		glyf_Contour contour;
		glyf_iContour.initCapN(&contour, contourdump && contourdump->type == json_array
		                                     ? contourdump->u.array.length
		                                     : 1);
		if (contourdump && contourdump->type == json_array) {
			for (shapeid_t k = 0; k < contourdump->u.array.length; k++) {
				glyf_iContour.push(&contour, glyf_parse_point(contourdump->u.array.values[k]));
			}
		}
		glyf_iContourList.push(&g->contours, contour);
	}
}

static glyf_ComponentReference glyf_parse_reference(json_value *refdump) {
	json_value *_gname = json_obj_get_type(refdump, "glyph", json_string);
	glyf_ComponentReference ref = glyf_iComponentReference.empty();
	if (_gname) {
		ref.glyph = Handle.fromName(sdsnewlen(_gname->u.string.ptr, _gname->u.string.length));
		iVQ.replace(&ref.x, json_vqOf(json_obj_get(refdump, "x"), NULL));
		iVQ.replace(&ref.y, json_vqOf(json_obj_get(refdump, "y"), NULL));
		ref.a = json_obj_getnum_fallback(refdump, "a", 1.0);
		ref.b = json_obj_getnum_fallback(refdump, "b", 0.0);
		ref.c = json_obj_getnum_fallback(refdump, "c", 0.0);
		ref.d = json_obj_getnum_fallback(refdump, "d", 1.0);
		ref.roundToGrid = json_obj_getbool(refdump, "roundToGrid");
		ref.useMyMetrics = json_obj_getbool(refdump, "useMyMetrics");
		if (json_obj_getbool(refdump, "isAnchored")) {
			ref.isAnchored = REF_ANCHOR_XY;
			ref.inner = json_obj_getint(refdump, "inner");
			ref.outer = json_obj_getint(refdump, "outer");
		}
	} else {
		// Invalid glyph references
		ref.glyph.name = NULL;
		iVQ.replace(&ref.x, iVQ.createStill(0));
		iVQ.replace(&ref.y, iVQ.createStill(0));
		ref.a = 1.0;
		ref.b = 0.0;
		ref.c = 0.0;
		ref.d = 1.0;
		ref.roundToGrid = false;
		ref.useMyMetrics = false;
	}
	return ref;
}
static void glyf_parse_references(json_value *col, glyf_Glyph *g) {
	if (!col) { return; }
	for (shapeid_t j = 0; j < col->u.array.length; j++) {
		glyf_iReferenceList.push(&g->references, glyf_parse_reference(col->u.array.values[j]));
	}
}

static void makeInstrsForGlyph(void *_g, uint8_t *instrs, uint32_t len) {
	glyf_Glyph *g = (glyf_Glyph *)_g;
	g->instructionsLength = len;
	g->instructions = instrs;
}
static void wrongInstrsForGlyph(void *_g, char *reason, int pos) {
	glyf_Glyph *g = (glyf_Glyph *)_g;
	fprintf(stderr, "[OTFCC] TrueType instructions parse error : %s, at %d in /%s\n", reason, pos,
	        g->name);
}

static void parse_stems(json_value *sd, glyf_StemDefList *stems) {
	if (!sd) return;
	for (shapeid_t j = 0; j < sd->u.array.length; j++) {
		json_value *s = sd->u.array.values[j];
		if (s->type != json_object) continue;
		glyf_PostscriptStemDef sdef;
		sdef.map = 0;
		sdef.position = json_obj_getnum(s, "position");
		sdef.width = json_obj_getnum(s, "width");
		glyf_iStemDefList.push(stems, sdef);
	}
}
static void parse_maskbits(bool *arr, json_value *bits) {
	if (!bits) {
		for (shapeid_t j = 0; j < 0x100; j++) {
			arr[j] = false;
		}
	} else {
		for (shapeid_t j = 0; j < 0x100 && j < bits->u.array.length; j++) {
			json_value *b = bits->u.array.values[j];
			switch (b->type) {
				case json_boolean:
					arr[j] = b->u.boolean;
					break;
				case json_integer:
					arr[j] = b->u.integer;
					break;
				case json_double:
					arr[j] = b->u.dbl;
					break;
				default:
					arr[j] = false;
			}
		}
	}
}
static void parse_masks(json_value *md, glyf_MaskList *masks) {
	if (!md) return;
	for (shapeid_t j = 0; j < md->u.array.length; j++) {
		json_value *m = md->u.array.values[j];
		if (m->type != json_object) continue;

		glyf_PostscriptHintMask mask;
		mask.pointsBefore = json_obj_getint(m, "pointsBefore");
		mask.contoursBefore = json_obj_getint(m, "contoursBefore");
		parse_maskbits(&(mask.maskH[0]), json_obj_get_type(m, "maskH", json_array));
		parse_maskbits(&(mask.maskV[0]), json_obj_get_type(m, "maskV", json_array));
		glyf_iMaskList.push(masks, mask);
	}
}

static glyf_Glyph *otfcc_glyf_parse_glyph(json_value *glyphdump, otfcc_GlyphOrderEntry *order_entry,
                                          const otfcc_Options *options) {
	glyf_Glyph *g = otfcc_newGlyf_glyph();
	g->name = sdsdup(order_entry->name);
	iVQ.replace(&g->advanceWidth, json_vqOf(json_obj_get(glyphdump, "advanceWidth"), NULL));
	iVQ.replace(&g->horizontalOrigin, json_vqOf(json_obj_get(glyphdump, "horizontalOrigin"), NULL));
	iVQ.replace(&g->advanceHeight, json_vqOf(json_obj_get(glyphdump, "advanceHeight"), NULL));
	iVQ.replace(&g->verticalOrigin, json_vqOf(json_obj_get(glyphdump, "verticalOrigin"), NULL));
	glyf_parse_contours(json_obj_get_type(glyphdump, "contours", json_array), g);
	glyf_parse_references(json_obj_get_type(glyphdump, "references", json_array), g);
	if (!options->ignore_hints) {
		parse_ttinstr(json_obj_get(glyphdump, "instructions"), g, makeInstrsForGlyph,
		              wrongInstrsForGlyph);
		parse_stems(json_obj_get_type(glyphdump, "stemH", json_array), &g->stemH);
		parse_stems(json_obj_get_type(glyphdump, "stemV", json_array), &g->stemV);
		parse_masks(json_obj_get_type(glyphdump, "hintMasks", json_array), &g->hintMasks);
		parse_masks(json_obj_get_type(glyphdump, "contourMasks", json_array), &g->contourMasks);
		g->yPel = json_obj_getint(glyphdump, "LTSH_yPel");
	}
	// Glyph data of other tables
	g->fdSelect = Handle.fromName(json_obj_getsds(glyphdump, "CFF_fdSelect"));
	if (!g->yPel) { g->yPel = json_obj_getint(glyphdump, "yPel"); }
	return g;
}

table_glyf *otfcc_parseGlyf(const json_value *root, otfcc_GlyphOrder *glyph_order,
                            const otfcc_Options *options) {
	if (root->type != json_object || !glyph_order) return NULL;
	table_glyf *glyf = NULL;
	json_value *table;
	if ((table = json_obj_get_type(root, "glyf", json_object))) {
		loggedStep("glyf") {
			glyphid_t numGlyphs = table->u.object.length;
			glyf = table_iGlyf.createN(numGlyphs);
			for (glyphid_t j = 0; j < numGlyphs; j++) {
				sds gname = sdsnewlen(table->u.object.values[j].name,
				                      table->u.object.values[j].name_length);
				json_value *glyphdump = table->u.object.values[j].value;
				otfcc_GlyphOrderEntry *order_entry = NULL;
				HASH_FIND(hhName, glyph_order->byName, gname, sdslen(gname), order_entry);
				if (glyphdump->type == json_object && order_entry &&
				    !glyf->items[order_entry->gid]) {
					glyf->items[order_entry->gid] =
					    otfcc_glyf_parse_glyph(glyphdump, order_entry, options);
				}
				json_value_free(glyphdump);
				json_value *v = json_null_new();
				v->parent = table;
				table->u.object.values[j].value = v;
				sdsfree(gname);
			}
		}
		return glyf;
	}
	return NULL;
}
