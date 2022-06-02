#include "GDEF.h"

#include "otl/private.h"

caryll_ElementInterfaceOf(otl_CaretValue) otl_iCaretValue = {
    .init = NULL,
    .copy = NULL,
    .dispose = NULL,
};
caryll_standardVectorImpl(otl_CaretValueList, otl_CaretValue, otl_iCaretValue, otl_iCaretValueList);

static INLINE void initGdefLigCaretRec(otl_CaretValueRecord *v) {
	v->glyph = Handle.empty();
	otl_iCaretValueList.init(&v->carets);
}
static void deleteGdefLigCaretRec(otl_CaretValueRecord *v) {
	Handle.dispose(&v->glyph);
	otl_iCaretValueList.dispose(&v->carets);
}
caryll_ElementInterfaceOf(otl_CaretValueRecord) otl_iCaretValueRecord = {
    .init = initGdefLigCaretRec,
    .copy = NULL,
    .dispose = deleteGdefLigCaretRec,
};
caryll_standardVectorImpl(otl_LigCaretTable, otl_CaretValueRecord, otl_iCaretValueRecord,
                          otl_iLigCaretTable);

static INLINE void initGDEF(table_GDEF *gdef) {
	gdef->glyphClassDef = NULL;
	gdef->markAttachClassDef = NULL;
	otl_iLigCaretTable.init(&gdef->ligCarets);
}
static INLINE void disposeGDEF(table_GDEF *gdef) {
	if (!gdef) return;
	if (gdef->glyphClassDef) ClassDef.free(gdef->glyphClassDef);
	if (gdef->markAttachClassDef) ClassDef.free(gdef->markAttachClassDef);
	otl_iLigCaretTable.dispose(&gdef->ligCarets);
}

caryll_standardRefType(table_GDEF, table_iGDEF, initGDEF, disposeGDEF);

static otl_CaretValue readCaretValue(const font_file_pointer data, uint32_t tableLength,
                                     uint32_t offset) {
	otl_CaretValue v;
	v.format = 0;
	v.coordiante = 0;
	v.pointIndex = 0xFFFF;
	checkLength(offset + 4);

	v.format = read_16u(data + offset);
	if (v.format == 2) { // attach to glyph point
		v.pointIndex = read_16u(data + offset + 2);
	} else {
		v.coordiante = (pos_t)read_16u(data + offset + 2);
	}
FAIL:
	return v;
}
static otl_CaretValueRecord readLigCaretRecord(const font_file_pointer data, uint32_t tableLength,
                                               uint32_t offset) {
	otl_CaretValueRecord g;
	otl_iCaretValueRecord.init(&g);
	checkLength(offset + 2);
	shapeid_t caretCount = read_16u(data + offset);
	checkLength(offset + 2 + caretCount * 2);

	for (glyphid_t j = 0; j < caretCount; j++) {
		otl_iCaretValueList.push(
		    &g.carets,
		    readCaretValue(data, tableLength, offset + read_16u(data + offset + 2 + j * 2)));
	}
FAIL:;
	return g;
}

table_GDEF *otfcc_readGDEF(const otfcc_Packet packet, const otfcc_Options *options) {
	table_GDEF *gdef = NULL;
	FOR_TABLE('GDEF', table) {
		font_file_pointer data = table.data;
		uint32_t tableLength = table.length;
		checkLength(12);
		gdef = table_iGDEF.create();
		uint16_t classdefOffset = read_16u(data + 4);
		if (classdefOffset) {
			gdef->glyphClassDef = ClassDef.read(data, tableLength, classdefOffset);
		}
		uint16_t ligCaretOffset = read_16u(data + 8);
		if (ligCaretOffset) {
			checkLength(ligCaretOffset + 4);
			otl_Coverage *cov =
			    Coverage.read(data, tableLength, ligCaretOffset + read_16u(data + ligCaretOffset));
			if (!cov || cov->numGlyphs != read_16u(data + ligCaretOffset + 2)) goto FAIL;
			checkLength(ligCaretOffset + 4 + cov->numGlyphs * 2);
			for (glyphid_t j = 0; j < cov->numGlyphs; j++) {
				otl_CaretValueRecord v = readLigCaretRecord(
				    data, tableLength,
				    ligCaretOffset + read_16u(data + ligCaretOffset + 4 + j * 2));
				v.glyph = Handle.dup(cov->glyphs[j]);
				otl_iLigCaretTable.push(&gdef->ligCarets, v);
			}
			Coverage.free(cov);
		}
		uint16_t markAttachDefOffset = read_16u(data + 10);
		if (markAttachDefOffset) {
			gdef->markAttachClassDef = ClassDef.read(data, tableLength, markAttachDefOffset);
		}
		return gdef;

	FAIL:
		DELETE(table_iGDEF.free, gdef);
	}
	return gdef;
}

static json_value *dumpGDEFLigCarets(const table_GDEF *gdef) {
	json_value *_carets = json_object_new(gdef->ligCarets.length);
	for (glyphid_t j = 0; j < gdef->ligCarets.length; j++) {
		sds name = gdef->ligCarets.items[j].glyph.name;
		json_value *_record = json_array_new(gdef->ligCarets.items[j].carets.length);

		for (glyphid_t k = 0; k < gdef->ligCarets.items[j].carets.length; k++) {
			json_value *_cv = json_object_new(1);
			if (gdef->ligCarets.items[j].carets.items[k].format == 2) {
				json_object_push(
				    _cv, "atPoint",
				    json_integer_new(gdef->ligCarets.items[j].carets.items[k].pointIndex));
			} else {
				json_object_push(
				    _cv, "at",
				    json_integer_new(gdef->ligCarets.items[j].carets.items[k].coordiante));
			}
			json_array_push(_record, _cv);
		}
		json_object_push(_carets, name, preserialize(_record));
	}
	return _carets;
}

void otfcc_dumpGDEF(const table_GDEF *gdef, json_value *root, const otfcc_Options *options) {
	if (!gdef) return;
	loggedStep("GDEF") {
		json_value *_gdef = json_object_new(4);
		if (gdef->glyphClassDef) {
			json_object_push(_gdef, "glyphClassDef", ClassDef.dump(gdef->glyphClassDef));
		}
		if (gdef->markAttachClassDef) {
			json_object_push(_gdef, "markAttachClassDef", ClassDef.dump(gdef->markAttachClassDef));
		}
		if (gdef->ligCarets.length) {
			json_object_push(_gdef, "ligCarets", dumpGDEFLigCarets(gdef));
		}
		json_object_push(root, "GDEF", _gdef);
	}
}

static void ligCaretFromJson(const json_value *_carets, otl_LigCaretTable *lc) {
	if (!_carets || _carets->type != json_object) return;

	for (glyphid_t j = 0; j < _carets->u.object.length; j++) {
		json_value *a = _carets->u.object.values[j].value;
		if (!a || a->type != json_array) continue;
		otl_CaretValueRecord v;
		otl_iCaretValueRecord.init(&v);
		v.glyph = Handle.fromName(
		    sdsnewlen(_carets->u.object.values[j].name, _carets->u.object.values[j].name_length));
		shapeid_t caretCount = a->u.array.length;
		for (glyphid_t k = 0; k < caretCount; k++) {
			otl_CaretValue caret;
			caret.format = 1;
			caret.coordiante = 0;
			caret.pointIndex = 0xFFFF;
			json_value *_caret = a->u.array.values[k];
			if (_caret && _caret->type == json_object) {
				if (json_obj_get_type(_caret, "atPoint", json_integer)) {
					caret.format = 2;
					caret.pointIndex = json_obj_getint(_caret, "atPoint");
				} else {
					caret.coordiante = json_obj_getnum(_caret, "at");
				}
			}
			otl_iCaretValueList.push(&v.carets, caret);
		}
		otl_iLigCaretTable.push(lc, v);
	}
}

table_GDEF *otfcc_parseGDEF(const json_value *root, const otfcc_Options *options) {
	table_GDEF *gdef = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "GDEF", json_object))) {
		loggedStep("GDEF") {
			gdef = table_iGDEF.create();
			gdef->glyphClassDef = ClassDef.parse(json_obj_get(table, "glyphClassDef"));
			gdef->markAttachClassDef = ClassDef.parse(json_obj_get(table, "markAttachClassDef"));
			ligCaretFromJson(json_obj_get(table, "ligCarets"), &gdef->ligCarets);
		}
	}
	return gdef;
}

static bk_Block *writeLigCaretRec(otl_CaretValueRecord *cr) {
	bk_Block *bcr = bk_new_Block(b16, cr->carets.length, // CaretCount
	                             bkover);
	for (glyphid_t j = 0; j < cr->carets.length; j++) {
		bk_push(bcr, p16,
		        bk_new_Block(b16, cr->carets.items[j].format, // format
		                     b16,
		                     cr->carets.items[j].format == 2
		                         ? cr->carets.items[j].pointIndex           // Point index
		                         : (int16_t)cr->carets.items[j].coordiante, // X coordinate
		                     bkover),                                       // CaretValue
		        bkover);
	}
	return bcr;
}

static bk_Block *writeLigCarets(const otl_LigCaretTable *lc) {
	otl_Coverage *cov = Coverage.create();
	for (glyphid_t j = 0; j < lc->length; j++) {
		Coverage.push(cov, Handle.dup(lc->items[j].glyph));
	}
	bk_Block *lct = bk_new_Block(p16, bk_newBlockFromBuffer(Coverage.build(cov)), // Coverage
	                             b16, lc->length,                                 // LigGlyphCount
	                             bkover);
	for (glyphid_t j = 0; j < lc->length; j++) {
		bk_push(lct, p16, writeLigCaretRec(&(lc->items[j])), bkover);
	}
	Coverage.free(cov);
	return lct;
}

caryll_Buffer *otfcc_buildGDEF(const table_GDEF *gdef, const otfcc_Options *options) {
	if (!gdef) return NULL;
	bk_Block *bGlyphClassDef = NULL;
	bk_Block *bAttachList = NULL;
	bk_Block *bLigCaretList = NULL;
	bk_Block *bMarkAttachClassDef = NULL;

	if (gdef->glyphClassDef) {
		bGlyphClassDef = bk_newBlockFromBuffer(ClassDef.build(gdef->glyphClassDef));
	}
	if (gdef->ligCarets.length) { bLigCaretList = writeLigCarets(&gdef->ligCarets); }
	if (gdef->markAttachClassDef) {
		bMarkAttachClassDef = bk_newBlockFromBuffer(ClassDef.build(gdef->markAttachClassDef));
	}
	bk_Block *root = bk_new_Block(b32, 0x10000,             // Version
	                              p16, bGlyphClassDef,      // GlyphClassDef
	                              p16, bAttachList,         // AttachList
	                              p16, bLigCaretList,       // LigCaretList
	                              p16, bMarkAttachClassDef, // MarkAttachClassDef
	                              bkover);
	return bk_build_Block(root);
}
