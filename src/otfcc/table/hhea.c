#include "hhea.h"

#include "support/util.h"

static INLINE void initHhea(table_hhea *hhea) {
	memset(hhea, 0, sizeof(*hhea));
	hhea->version = 0x10000;
}
static INLINE void disposeHhea(MOVE table_hhea *hhea) {
	// trivial
}
caryll_standardRefType(table_hhea, table_iHhea, initHhea, disposeHhea);

table_hhea *otfcc_readHhea(const otfcc_Packet packet, const otfcc_Options *options) {
	FOR_TABLE('hhea', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;

		if (length < 36) {
			logWarning("table 'hhea' corrupted.\n");
		} else {
			table_hhea *hhea;
			NEW(hhea);
			hhea->version = read_32s(data);
			hhea->ascender = read_16u(data + 4);
			hhea->descender = read_16u(data + 6);
			hhea->lineGap = read_16u(data + 8);
			hhea->advanceWidthMax = read_16u(data + 10);
			hhea->minLeftSideBearing = read_16u(data + 12);
			hhea->minRightSideBearing = read_16u(data + 14);
			hhea->xMaxExtent = read_16u(data + 16);
			hhea->caretSlopeRise = read_16u(data + 18);
			hhea->caretSlopeRun = read_16u(data + 20);
			hhea->caretOffset = read_16u(data + 22);
			hhea->reserved[0] = read_16u(data + 24);
			hhea->reserved[1] = read_16u(data + 26);
			hhea->reserved[2] = read_16u(data + 28);
			hhea->reserved[3] = read_16u(data + 30);
			hhea->metricDataFormat = read_16u(data + 32);
			hhea->numberOfMetrics = read_16u(data + 34);
			return hhea;
		}
	}
	return NULL;
}

void otfcc_dumpHhea(const table_hhea *table, json_value *root, const otfcc_Options *options) {
	if (!table) return;
	loggedStep("hhea") {
		json_value *hhea = json_object_new(13);
		json_object_push(hhea, "version", json_double_new(otfcc_from_fixed(table->version)));
		json_object_push(hhea, "ascender", json_integer_new(table->ascender));
		json_object_push(hhea, "descender", json_integer_new(table->descender));
		json_object_push(hhea, "lineGap", json_integer_new(table->lineGap));
		json_object_push(hhea, "advanceWidthMax", json_integer_new(table->advanceWidthMax));
		json_object_push(hhea, "minLeftSideBearing", json_integer_new(table->minLeftSideBearing));
		json_object_push(hhea, "minRightSideBearing", json_integer_new(table->minRightSideBearing));
		json_object_push(hhea, "xMaxExtent", json_integer_new(table->xMaxExtent));
		json_object_push(hhea, "caretSlopeRise", json_integer_new(table->caretSlopeRise));
		json_object_push(hhea, "caretSlopeRun", json_integer_new(table->caretSlopeRun));
		json_object_push(hhea, "caretOffset", json_integer_new(table->caretOffset));
		json_object_push(root, "hhea", hhea);
	}
}

table_hhea *otfcc_parseHhea(const json_value *root, const otfcc_Options *options) {
	table_hhea *hhea = table_iHhea.create();
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "hhea", json_object))) {
		loggedStep("hhea") {
			hhea->version = otfcc_to_fixed(json_obj_getnum_fallback(table, "version", 0));
			hhea->ascender = json_obj_getnum_fallback(table, "ascender", 0);
			hhea->descender = json_obj_getnum_fallback(table, "descender", 0);
			hhea->lineGap = json_obj_getnum_fallback(table, "lineGap", 0);
			hhea->advanceWidthMax = json_obj_getnum_fallback(table, "advanceWidthMax", 0);
			hhea->minLeftSideBearing = json_obj_getnum_fallback(table, "minLeftSideBearing", 0);
			hhea->minRightSideBearing = json_obj_getnum_fallback(table, "minRightSideBearing", 0);
			hhea->xMaxExtent = json_obj_getnum_fallback(table, "xMaxExtent", 0);
			hhea->caretSlopeRise = json_obj_getnum_fallback(table, "caretSlopeRise", 0);
			hhea->caretSlopeRun = json_obj_getnum_fallback(table, "caretSlopeRun", 0);
			hhea->caretOffset = json_obj_getnum_fallback(table, "caretOffset", 0);
		}
	}
	return hhea;
}

caryll_Buffer *otfcc_buildHhea(const table_hhea *hhea, const otfcc_Options *options) {
	if (!hhea) return NULL;
	caryll_Buffer *buf = bufnew();
	bufwrite32b(buf, hhea->version);
	bufwrite16b(buf, hhea->ascender);
	bufwrite16b(buf, hhea->descender);
	bufwrite16b(buf, hhea->lineGap);
	bufwrite16b(buf, hhea->advanceWidthMax);
	bufwrite16b(buf, hhea->minLeftSideBearing);
	bufwrite16b(buf, hhea->minRightSideBearing);
	bufwrite16b(buf, hhea->xMaxExtent);
	bufwrite16b(buf, hhea->caretSlopeRise);
	bufwrite16b(buf, hhea->caretSlopeRun);
	bufwrite16b(buf, hhea->caretOffset);
	bufwrite16b(buf, hhea->reserved[0]);
	bufwrite16b(buf, hhea->reserved[1]);
	bufwrite16b(buf, hhea->reserved[2]);
	bufwrite16b(buf, hhea->reserved[3]);
	bufwrite16b(buf, 0);
	bufwrite16b(buf, hhea->numberOfMetrics);
	return buf;
}
