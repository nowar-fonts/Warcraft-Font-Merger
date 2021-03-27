#include "json-ident.h"
#include "support/otfcc-alloc.h"

#ifdef _MSC_VER
#include "winfns.h"
#endif

static bool compare_json_arrays(const json_value *a, const json_value *b) {
	for (uint16_t j = 0; j < a->u.array.length; j++) {
		if (!json_ident(a->u.array.values[j], b->u.array.values[j])) { return false; }
	}
	return true;
}

typedef struct {
	char *key;
	json_value *val;
	bool check;
	UT_hash_handle hh;
} json_obj_entry;
static bool compare_json_objects(const json_value *a, const json_value *b) {
	json_obj_entry *h = NULL;
	for (uint32_t j = 0; j < a->u.object.length; j++) {
		char *k = a->u.object.values[j].name;
		json_obj_entry *e = NULL;
		HASH_FIND_STR(h, k, e);
		if (!e) {
			NEW(e);
			e->key = strdup(k);
			e->val = a->u.object.values[j].value;
			e->check = false;
			HASH_ADD_STR(h, key, e);
		}
	}
	bool allcheck = true;
	for (uint32_t j = 0; j < b->u.object.length; j++) {
		char *k = b->u.object.values[j].name;
		json_obj_entry *e = NULL;
		HASH_FIND_STR(h, k, e);
		if (!e) {
			allcheck = false;
			break;
		} else {
			bool check = json_ident(e->val, b->u.object.values[j].value);
			if (!check) {
				allcheck = false;
				break;
			} else {
				e->check = true;
			}
		}
	}
	json_obj_entry *e, *tmp;
	HASH_ITER(hh, h, e, tmp) {
		allcheck = allcheck && e->check;
		HASH_DEL(h, e);
		FREE(e->key);
		FREE(e);
	}
	return allcheck;
}

bool json_ident(const json_value *a, const json_value *b) {
	if (!a && !b) return true;
	if (!a || !b) return false;
	if (a->type != b->type) return false;
	switch (a->type) {
		case json_none:
		case json_null:
			return true;
		case json_integer:
			return a->u.integer == b->u.integer;
		case json_double:
			return a->u.dbl == b->u.dbl;
		case json_boolean:
			return a->u.boolean == b->u.boolean;
		case json_string:
			return a->u.string.length == b->u.string.length && strcmp(a->u.string.ptr, b->u.string.ptr) == 0;
		case json_array:
			return a->u.array.length == b->u.array.length && compare_json_arrays(a, b);
		case json_object:
			return a->u.object.length == b->u.object.length && compare_json_objects(a, b);
		default:
			return false;
	}
}
