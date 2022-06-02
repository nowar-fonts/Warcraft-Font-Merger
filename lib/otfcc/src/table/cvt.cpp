#include "cvt.h"

#include "support/util.h"

static INLINE void disposeCvt(MOVE table_cvt *table) {
	if (table->words) FREE(table->words);
}

caryll_standardRefType(table_cvt, table_iCvt, disposeCvt);

table_cvt *otfcc_readCvt(const otfcc_Packet packet, const otfcc_Options *options, uint32_t tag) {
	table_cvt *t = NULL;
	FOR_TABLE(tag, table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		NEW(t);
		t->length = length >> 1;
		NEW(t->words, (t->length + 1));
		for (uint16_t j = 0; j < t->length; j++) {
			t->words[j] = read_16u(data + 2 * j);
		}
		return t;
	}
	return NULL;
}

void otfcc_dumpCvt(const table_cvt *table, json_value *root, const otfcc_Options *options,
                   const char *tag) {
	if (!table) return;
	loggedStep("cvt") {
		json_value *arr = json_array_new(table->length);
		for (uint16_t j = 0; j < table->length; j++) {
			json_array_push(arr, json_integer_new(table->words[j]));
		}
		json_object_push(root, tag, arr);
	}
}
table_cvt *otfcc_parseCvt(const json_value *root, const otfcc_Options *options, const char *tag) {
	table_cvt *t = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, tag, json_array))) {
		loggedStep("cvt") {
			// Meaningful CVT dump
			NEW(t);
			t->length = table->u.array.length;
			NEW(t->words, (t->length + 1));
			for (uint16_t j = 0; j < t->length; j++) {
				json_value *record = table->u.array.values[j];
				if (record->type == json_integer) {
					t->words[j] = record->u.integer;
				} else if (record->type == json_double) {
					t->words[j] = (uint16_t)record->u.dbl;
				} else {
					t->words[j] = 0;
				}
			}
		}
	} else if ((table = json_obj_get_type(root, tag, json_string))) {
		loggedStep("cvt") {
			// Bytes CVT dump
			NEW(t);
			size_t len;
			uint8_t *raw =
			    base64_decode((uint8_t *)table->u.string.ptr, table->u.string.length, &len);
			t->length = (uint32_t)(len >> 1);
			NEW(t->words, (t->length + 1));
			for (uint16_t j = 0; j < t->length; j++) {
				t->words[j] = read_16u(raw + 2 * j);
			}
			FREE(raw);
		}
	}
	return t;
}

caryll_Buffer *otfcc_buildCvt(const table_cvt *table, const otfcc_Options *options) {
	if (!table) return NULL;
	caryll_Buffer *buf = bufnew();
	for (uint16_t j = 0; j < table->length; j++) {
		bufwrite16b(buf, table->words[j]);
	}
	return buf;
}
