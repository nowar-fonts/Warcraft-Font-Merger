#include "support/util.h"
#include "otfcc/font.h"
#include "table/all.h"

static otfcc_font_subtype otfcc_decideFontSubtypeFromJson(const json_value *root) {
	if (json_obj_get_type(root, "CFF_", json_object) != NULL) {
		return FONTTYPE_CFF;
	} else {
		return FONTTYPE_TTF;
	}
}

// The default glyph_order object is completed using a two-step construction
enum { ORD_GLYPHORDER = 1, ORD_NOTDEF = 2, ORD_CMAP = 3, ORD_GLYF = 4 };

// Register a name->(orderType, orderEntry) map.
static void setOrderByName(otfcc_GlyphOrder *go, sds name, uint8_t orderType, uint32_t orderEntry) {
	otfcc_GlyphOrderEntry *s = NULL;
	HASH_FIND(hhName, go->byName, name, sdslen(name), s);
	if (!s) {
		NEW(s);
		s->gid = -1;
		s->name = name;
		s->orderType = orderType;
		s->orderEntry = orderEntry;
		HASH_ADD(hhName, go->byName, name[0], sdslen(s->name), s);
	} else if (s->orderType > orderType) {
		s->orderType = orderType;
		s->orderEntry = orderEntry;
	}
}

static int _byOrder(otfcc_GlyphOrderEntry *a, otfcc_GlyphOrderEntry *b) {
	if (a->orderType < b->orderType) return (-1);
	if (a->orderType > b->orderType) return (1);
	if (a->orderEntry < b->orderEntry) return (-1);
	if (a->orderEntry > b->orderEntry) return (1);
	return 0;
}

// Complete ClyphOrder
static void orderGlyphs(otfcc_GlyphOrder *go) {
	HASH_SRT(hhName, go->byName, _byOrder);
	otfcc_GlyphOrderEntry *current, *temp;
	glyphid_t gid = 0;
	HASH_ITER(hhName, go->byName, current, temp) {
		current->gid = gid;
		HASH_ADD(hhID, go->byGID, gid, sizeof(glyphid_t), current);
		gid += 1;
	}
}

static void escalateGlyphOrderByName(otfcc_GlyphOrder *go, sds name, uint8_t orderType,
                                     uint32_t orderEntry) {
	otfcc_GlyphOrderEntry *s = NULL;
	HASH_FIND(hhName, go->byName, name, sdslen(name), s);
	if (s && s->orderType > orderType) {
		s->orderType = orderType;
		s->orderEntry = orderEntry;
	}
}

static void placeOrderEntriesFromGlyf(json_value *table, otfcc_GlyphOrder *go) {
	for (uint32_t j = 0; j < table->u.object.length; j++) {
		sds gname =
		    sdsnewlen(table->u.object.values[j].name, table->u.object.values[j].name_length);
		if (strcmp(gname, ".notdef") == 0) {
			setOrderByName(go, gname, ORD_NOTDEF, 0);
		} else if (strcmp(gname, ".null") == 0) {
			setOrderByName(go, gname, ORD_NOTDEF, 1);
		} else {
			setOrderByName(go, gname, ORD_GLYF, j);
		}
	}
}
static void placeOrderEntriesFromCmap(json_value *table, otfcc_GlyphOrder *go) {
	for (uint32_t j = 0; j < table->u.object.length; j++) {
		sds unicodeStr =
		    sdsnewlen(table->u.object.values[j].name, table->u.object.values[j].name_length);
		json_value *item = table->u.object.values[j].value;
		int32_t unicode;
		if (sdslen(unicodeStr) > 2 && unicodeStr[0] == 'U' && unicodeStr[1] == '+') {
			unicode = strtol(unicodeStr + 2, NULL, 16);
		} else {
			unicode = atoi(unicodeStr);
		}
		sdsfree(unicodeStr);
		if (item->type == json_string && unicode > 0 &&
		    unicode <= 0x10FFFF) { // a valid unicode codepoint
			sds gname = sdsnewlen(item->u.string.ptr, item->u.string.length);
			escalateGlyphOrderByName(go, gname, ORD_CMAP, unicode);
			sdsfree(gname);
		}
	}
}
static void placeOrderEntriesFromSubtable(json_value *table, otfcc_GlyphOrder *go, bool zeroOnly) {
	uint32_t uplimit = table->u.array.length;
	if (uplimit >= 1 && zeroOnly) { uplimit = 1; }
	for (uint32_t j = 0; j < uplimit; j++) {
		json_value *item = table->u.array.values[j];
		if (item->type == json_string) {
			sds gname = sdsnewlen(item->u.string.ptr, item->u.string.length);
			escalateGlyphOrderByName(go, gname, ORD_GLYPHORDER, j);
			sdsfree(gname);
		}
	}
}

static otfcc_GlyphOrder *parseGlyphOrder(const json_value *root, const otfcc_Options *options) {
	otfcc_GlyphOrder *go = GlyphOrder.create();
	if (root->type != json_object) return go;
	json_value *table;

	if ((table = json_obj_get_type(root, "glyf", json_object))) {
		placeOrderEntriesFromGlyf(table, go);
		if ((table = json_obj_get_type(root, "cmap", json_object))) {
			placeOrderEntriesFromCmap(table, go);
		}
		if ((table = json_obj_get_type(root, "glyph_order", json_array))) {
			bool ignoreGlyphOrder = options->ignore_glyph_order;
			if (ignoreGlyphOrder && !!json_obj_get_type(root, "SVG_", json_array)) {
				logNotice("OpenType SVG table detected. Glyph order is preserved.");
				ignoreGlyphOrder = false;
			}
			placeOrderEntriesFromSubtable(table, go, ignoreGlyphOrder);
		}
	}
	orderGlyphs(go);
	return go;
}

static otfcc_Font *readJson(void *_root, uint32_t index, const otfcc_Options *options) {
	const json_value *root = (json_value *)_root;
	otfcc_Font *font = otfcc_iFont.create();
	if (!font) return NULL;
	font->subtype = otfcc_decideFontSubtypeFromJson(root);
	font->glyph_order = parseGlyphOrder(root, options);
	font->glyf = otfcc_parseGlyf(root, font->glyph_order, options);
	font->CFF_ = otfcc_parseCFF(root, options);
	font->head = otfcc_parseHead(root, options);
	font->hhea = otfcc_parseHhea(root, options);
	font->OS_2 = otfcc_parseOS_2(root, options);
	font->maxp = otfcc_parseMaxp(root, options);
	font->post = otfcc_parsePost(root, options);
	font->name = otfcc_parseName(root, options);
	font->meta = otfcc_parseMeta(root, options);
	font->cmap = otfcc_parseCmap(root, options);
	if (!options->ignore_hints) {
		font->fpgm = otfcc_parseFpgmPrep(root, options, "fpgm");
		font->prep = otfcc_parseFpgmPrep(root, options, "prep");
		font->cvt_ = otfcc_parseCvt(root, options, "cvt_");
		font->gasp = otfcc_parseGasp(root, options);
	}
	font->VDMX = otfcc_parseVDMX(root, options);
	font->vhea = otfcc_parseVhea(root, options);
	if (font->glyf) {
		font->GSUB = otfcc_parseOtl(root, options, "GSUB");
		font->GPOS = otfcc_parseOtl(root, options, "GPOS");
		font->GDEF = otfcc_parseGDEF(root, options);
	}
	font->BASE = otfcc_parseBASE(root, options);
	font->CPAL = otfcc_parseCPAL(root, options);
	font->COLR = otfcc_parseCOLR(root, options);
	font->SVG_ = otfcc_parseSVG(root, options);

	font->TSI_01 = otfcc_parseTSI(root, options, "TSI_01");
	font->TSI_23 = otfcc_parseTSI(root, options, "TSI_23");
	font->TSI5 = otfcc_parseTSI5(root, options);

	return font;
}
static INLINE void freeReader(otfcc_IFontBuilder *self) {
	free(self);
}
otfcc_IFontBuilder *otfcc_newJsonReader() {
	otfcc_IFontBuilder *reader;
	NEW(reader);
	reader->read = readJson;
	reader->free = freeReader;
	return reader;
}
