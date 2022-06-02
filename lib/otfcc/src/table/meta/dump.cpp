#include "../meta.h"

#include "support/util.h"

static INLINE bool isStringTag(uint32_t tag) {
	return tag == 'dlng' || tag == 'slng';
}

void otfcc_dumpMeta(const table_meta *meta, json_value *root, const otfcc_Options *options) {
	if (!meta) return;
	loggedStep("meta") {
		json_value *_meta = json_object_new(3);
		json_object_push(_meta, "version", json_integer_new(meta->version));
		json_object_push(_meta, "flags", json_integer_new(meta->flags));
		json_value *_entries = json_array_new(meta->entries.length);
		json_object_push(_meta, "entries", _entries);
		foreach (meta_Entry *e, meta->entries) {
			json_value *_e = json_object_new(2);
			char _tag[4];
			tag2str(e->tag, _tag);
			json_object_push(_e, "tag", json_string_new_length(4, _tag));
			if (isStringTag(e->tag)) {
				json_object_push(_e, "string",
				                 json_string_new_length((uint32_t)sdslen(e->data), e->data));
			} else {
				size_t outLen = 0;
				uint8_t *out = base64_encode((uint8_t *)e->data, sdslen(e->data), &outLen);
				json_object_push(_e, "base64",
				                 json_string_new_length((uint32_t)outLen, (char *)out));
				FREE(out);
			}
			json_array_push(_entries, _e);
		}
		json_object_push(root, "meta", _meta);
	}
}
