#include "../meta.h"

#include "support/util.h"

sds parseMetaData(const json_value *v) {
	if (v->type == json_string) {
		return sdsnewlen(v->u.string.ptr, v->u.string.length);
	} else if (v->type == json_object) {
		// string entry
		json_value *_string = json_obj_get_type(v, "string", json_string);
		if (_string) { return sdsnewlen(_string->u.string.ptr, _string->u.string.length); }

		// base64 entry
		json_value *_base64 = json_obj_get_type(v, "base64", json_string);
		if (_base64) {
			size_t strLen = 0;
			char *str = (char *)base64_decode((uint8_t *)_base64->u.string.ptr,
			                                  _base64->u.string.length, &strLen);
			sds s = sdsnewlen(str, strLen);
			FREE(str);
			return s;
		}
	}
	return NULL;
}

table_meta *otfcc_parseMeta(const json_value *root, const otfcc_Options *options) {
	json_value *_meta = NULL;
	if (!(_meta = json_obj_get_type(root, "meta", json_object))) return NULL;
	json_value *_meta_entries = NULL;
	if (!(_meta_entries = json_obj_get_type(_meta, "entries", json_array))) return NULL;
	table_meta *meta = table_iMeta.create();
	loggedStep("meta") {
		for (size_t j = 0; j < _meta_entries->u.array.length; j++) {

			json_value *_e = _meta_entries->u.array.values[j];
			json_value *_tag = json_obj_get_type(_e, "tag", json_string);
			if (!_tag || _tag->u.string.length != 4) continue;

			uint32_t tag = str2tag(_tag->u.string.ptr);
			sds str = parseMetaData(_e);
			if (!str) continue;
			meta_iEntries.push(&meta->entries, (meta_Entry){.tag = tag, .data = str});
		}
	}
	return meta;
}
