#include "TSI5.h"
#include "support/util.h"

table_TSI5 *otfcc_readTSI5(const otfcc_Packet packet, const otfcc_Options *options) {
	FOR_TABLE('TSI5', table) {
		table_TSI5 *tsi5 = otl_iClassDef.create();
		for (glyphid_t j = 0; j * 2 < table.length; j++) {
			otl_iClassDef.push(tsi5, Handle.fromIndex(j), read_16u(table.data + j * 2));
		}
		return tsi5;
	}
	return NULL;
}
void otfcc_dumpTSI5(const table_TSI5 *table, json_value *root, const otfcc_Options *options) {
	if (!table) return;
	json_object_push(root, "TSI5", otl_iClassDef.dump(table));
}
table_TSI5 *otfcc_parseTSI5(const json_value *root, const otfcc_Options *options) {
	json_value *_tsi = NULL;
	if (!(_tsi = json_obj_get_type(root, "TSI5", json_object))) return NULL;
	return otl_iClassDef.parse(_tsi);
}
caryll_Buffer *otfcc_buildTSI5(const table_TSI5 *tsi5, const otfcc_Options *options,
                               glyphid_t numGlyphs) {
	if (!tsi5) return NULL;
	uint16_t *tsi5cls;
	NEW_CLEAN_N(tsi5cls, numGlyphs);
	for (glyphid_t j = 0; j < tsi5->numGlyphs; j++) {
		if (tsi5->glyphs[j].index < numGlyphs) {
			tsi5cls[tsi5->glyphs[j].index] = tsi5->classes[j];
		}
	}
	caryll_Buffer *buf = bufnew();
	for (glyphid_t j = 0; j < numGlyphs; j++) {
		bufwrite16b(buf, tsi5cls[j]);
	}
	FREE(tsi5cls);
	return buf;
}
