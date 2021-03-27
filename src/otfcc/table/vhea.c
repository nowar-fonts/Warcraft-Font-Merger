#include "vhea.h"

#include "support/util.h"

static INLINE void initVhea(table_vhea *vhea) {
	memset(vhea, 0, sizeof(*vhea));
	vhea->version = 0x10000;
}
static INLINE void disposeVhea(MOVE table_vhea *vhea) {
	// trivial
}
caryll_standardRefType(table_vhea, table_iVhea, initVhea, disposeVhea);

table_vhea *otfcc_readVhea(const otfcc_Packet packet, const otfcc_Options *options) {
	table_vhea *vhea = NULL;
	FOR_TABLE('vhea', table) {
		font_file_pointer data = table.data;
		size_t length = table.length;
		if (length >= 36) {
			NEW(vhea);
			vhea->version = read_32s(data);
			vhea->ascent = read_16s(data + 4);
			vhea->descent = read_16s(data + 6);
			vhea->lineGap = read_16s(data + 8);
			vhea->advanceHeightMax = read_16s(data + 10);
			vhea->minTop = read_16s(data + 12);
			vhea->minBottom = read_16s(data + 14);
			vhea->yMaxExtent = read_16s(data + 16);
			vhea->caretSlopeRise = read_16s(data + 18);
			vhea->caretSlopeRun = read_16s(data + 20);
			vhea->caretOffset = read_16s(data + 22);
			vhea->dummy0 = 0;
			vhea->dummy1 = 0;
			vhea->dummy2 = 0;
			vhea->dummy3 = 0;
			vhea->metricDataFormat = 0;
			vhea->numOfLongVerMetrics = read_16u(data + 34);
			return vhea;
		} else {
			logWarning("Table 'vhea' corrupted.")
		}
	}
	return NULL;
}
void otfcc_dumpVhea(const table_vhea *table, json_value *root, const otfcc_Options *options) {
	if (!table) return;
	json_value *vhea = json_object_new(11);
	loggedStep("vhea") {
		json_object_push(vhea, "version", json_double_new(otfcc_from_fixed(table->version)));
		json_object_push(vhea, "ascent", json_integer_new(table->ascent));
		json_object_push(vhea, "descent", json_integer_new(table->descent));
		json_object_push(vhea, "lineGap", json_integer_new(table->lineGap));
		json_object_push(vhea, "advanceHeightMax", json_integer_new(table->advanceHeightMax));
		json_object_push(vhea, "minTop", json_integer_new(table->minTop));
		json_object_push(vhea, "minBottom", json_integer_new(table->minBottom));
		json_object_push(vhea, "yMaxExtent", json_integer_new(table->yMaxExtent));
		json_object_push(vhea, "caretSlopeRise", json_integer_new(table->caretSlopeRise));
		json_object_push(vhea, "caretSlopeRun", json_integer_new(table->caretSlopeRun));
		json_object_push(vhea, "caretOffset", json_integer_new(table->caretOffset));
		json_object_push(root, "vhea", vhea);
	}
}

table_vhea *otfcc_parseVhea(const json_value *root, const otfcc_Options *options) {
	table_vhea *vhea = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "vhea", json_object))) {
		vhea = table_iVhea.create();
		if (!vhea) return NULL;
		loggedStep("vhea") {
			vhea->version = otfcc_to_fixed(json_obj_getnum(table, "version"));
			vhea->ascent = json_obj_getnum_fallback(table, "ascent", 0);
			vhea->descent = json_obj_getnum_fallback(table, "descent", 0);
			vhea->lineGap = json_obj_getnum_fallback(table, "lineGap", 0);
			vhea->advanceHeightMax = json_obj_getnum_fallback(table, "advanceHeightMax", 0);
			vhea->minTop = json_obj_getnum_fallback(table, "minTop", 0);
			vhea->minBottom = json_obj_getnum_fallback(table, "minBottom", 0);
			vhea->yMaxExtent = json_obj_getnum_fallback(table, "yMaxExtent", 0);
			vhea->caretSlopeRise = json_obj_getnum_fallback(table, "caretSlopeRise", 0);
			vhea->caretSlopeRun = json_obj_getnum_fallback(table, "caretSlopeRun", 0);
			vhea->caretOffset = json_obj_getnum_fallback(table, "caretOffset", 0);
		}
	}
	return vhea;
}

caryll_Buffer *otfcc_buildVhea(const table_vhea *vhea, const otfcc_Options *options) {
	if (!vhea) return NULL;
	caryll_Buffer *buf = bufnew();
	bufwrite32b(buf, vhea->version);
	bufwrite16b(buf, vhea->ascent);
	bufwrite16b(buf, vhea->descent);
	bufwrite16b(buf, vhea->lineGap);
	bufwrite16b(buf, vhea->advanceHeightMax);
	bufwrite16b(buf, vhea->minTop);
	bufwrite16b(buf, vhea->minBottom);
	bufwrite16b(buf, vhea->yMaxExtent);
	bufwrite16b(buf, vhea->caretSlopeRise);
	bufwrite16b(buf, vhea->caretSlopeRun);
	bufwrite16b(buf, vhea->caretOffset);
	bufwrite16b(buf, 0);
	bufwrite16b(buf, 0);
	bufwrite16b(buf, 0);
	bufwrite16b(buf, 0);
	bufwrite16b(buf, 0);
	bufwrite16b(buf, vhea->numOfLongVerMetrics);
	return buf;
}
