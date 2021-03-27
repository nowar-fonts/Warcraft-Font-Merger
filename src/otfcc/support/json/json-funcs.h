#ifndef CARYLL_SUPPORT_JSON_FUNCS_H
#define CARYLL_SUPPORT_JSON_FUNCS_H

#include <stdint.h>
#include <math.h>

#include "dep/json-builder.h"
#include "dep/json.h"
#include "dep/sds.h"

#include "caryll/ownership.h"
#include "otfcc/primitives.h"
#include "otfcc/vf/vq.h"
#include "otfcc/table/fvar.h"

#ifndef INLINE
#ifdef _MSC_VER
#define INLINE __forceinline /* use __forceinline (VC++ specific) */
#else
#define INLINE inline /* use standard inline */
#endif
#endif

static INLINE json_value *preserialize(MOVE json_value *x);

static INLINE json_value *json_obj_get(const json_value *obj, const char *key) {
	if (!obj || obj->type != json_object) return NULL;
	for (uint32_t _k = 0; _k < obj->u.object.length; _k++) {
		char *ck = obj->u.object.values[_k].name;
		if (strcmp(ck, key) == 0) return obj->u.object.values[_k].value;
	}
	return NULL;
}
static INLINE json_value *json_obj_get_type(const json_value *obj, const char *key,
                                            const json_type type) {
	json_value *v = json_obj_get(obj, key);
	if (v && v->type == type) return v;
	return NULL;
}
static INLINE sds json_obj_getsds(const json_value *obj, const char *key) {
	json_value *v = json_obj_get_type(obj, key, json_string);
	if (!v)
		return NULL;
	else
		return sdsnewlen(v->u.string.ptr, v->u.string.length);
}
static INLINE char *json_obj_getstr_share(const json_value *obj, const char *key) {
	json_value *v = json_obj_get_type(obj, key, json_string);
	if (!v)
		return NULL;
	else
		return v->u.string.ptr;
}

static INLINE json_value *json_object_push_tag(json_value *a, uint32_t tag, json_value *b) {
	char tags[4] = {(tag & 0xff000000) >> 24, (tag & 0xff0000) >> 16, (tag & 0xff00) >> 8,
	                (tag & 0xff)};
	return json_object_push_length(a, 4, tags, b);
}

// Coordinates, VV and VQ
static INLINE double json_numof(const json_value *cv) {
	if (cv && cv->type == json_integer) return cv->u.integer;
	if (cv && cv->type == json_double) return cv->u.dbl;
	return 0;
}
static INLINE json_value *json_new_position(pos_t z) {
	if (round(z) == z) {
		return json_integer_new(z);
	} else {
		return json_double_new(z);
	}
}
json_value *json_new_VQRegion_Explicit(const vq_Region *rs, const table_fvar *fvar);
json_value *json_new_VQRegion(const vq_Region *rs, const table_fvar *fvar);
json_value *json_new_VQ(const VQ z, const table_fvar *fvar);
json_value *json_new_VV(const VV x, const table_fvar *fvar);
json_value *json_new_VVp(const VV *x, const table_fvar *fvar);
VQ json_vqOf(const json_value *cv, const table_fvar *fvar);

static INLINE double json_obj_getnum(const json_value *obj, const char *key) {
	if (!obj || obj->type != json_object) return 0.0;
	for (uint32_t _k = 0; _k < obj->u.object.length; _k++) {
		char *ck = obj->u.object.values[_k].name;
		json_value *cv = obj->u.object.values[_k].value;
		if (strcmp(ck, key) == 0) {
			if (cv && cv->type == json_integer) return cv->u.integer;
			if (cv && cv->type == json_double) return cv->u.dbl;
		}
	}
	return 0.0;
}
static INLINE int32_t json_obj_getint(const json_value *obj, const char *key) {
	if (!obj || obj->type != json_object) return 0;
	for (uint32_t _k = 0; _k < obj->u.object.length; _k++) {
		char *ck = obj->u.object.values[_k].name;
		json_value *cv = obj->u.object.values[_k].value;
		if (strcmp(ck, key) == 0) {
			if (cv && cv->type == json_integer) return (int32_t)cv->u.integer;
			if (cv && cv->type == json_double) return cv->u.dbl;
		}
	}
	return 0;
}
static INLINE double json_obj_getnum_fallback(const json_value *obj, const char *key,
                                              double fallback) {
	if (!obj || obj->type != json_object) return fallback;
	for (uint32_t _k = 0; _k < obj->u.object.length; _k++) {
		char *ck = obj->u.object.values[_k].name;
		json_value *cv = obj->u.object.values[_k].value;
		if (strcmp(ck, key) == 0) {
			if (cv && cv->type == json_integer) return cv->u.integer;
			if (cv && cv->type == json_double) return cv->u.dbl;
		}
	}
	return fallback;
}
static INLINE int32_t json_obj_getint_fallback(const json_value *obj, const char *key,
                                               int32_t fallback) {
	if (!obj || obj->type != json_object) return fallback;
	for (uint32_t _k = 0; _k < obj->u.object.length; _k++) {
		char *ck = obj->u.object.values[_k].name;
		json_value *cv = obj->u.object.values[_k].value;
		if (strcmp(ck, key) == 0) {
			if (cv && cv->type == json_integer) return (int32_t)cv->u.integer;
			if (cv && cv->type == json_double) return cv->u.dbl;
		}
	}
	return fallback;
}
static INLINE bool json_boolof(const json_value *cv) {
	if (cv && cv->type == json_boolean) return cv->u.boolean;
	return false;
}
static INLINE bool json_obj_getbool(const json_value *obj, const char *key) {
	if (!obj || obj->type != json_object) return false;
	for (uint32_t _k = 0; _k < obj->u.object.length; _k++) {
		char *ck = obj->u.object.values[_k].name;
		json_value *cv = obj->u.object.values[_k].value;
		if (strcmp(ck, key) == 0) {
			if (cv && cv->type == json_boolean) return cv->u.boolean;
		}
	}
	return false;
}
static INLINE bool json_obj_getbool_fallback(const json_value *obj, const char *key,
                                             bool fallback) {
	if (!obj || obj->type != json_object) return fallback;
	for (uint32_t _k = 0; _k < obj->u.object.length; _k++) {
		char *ck = obj->u.object.values[_k].name;
		json_value *cv = obj->u.object.values[_k].value;
		if (strcmp(ck, key) == 0) {
			if (cv && cv->type == json_boolean) return cv->u.boolean;
		}
	}
	return fallback;
}

static INLINE json_value *json_from_sds(const sds str) {
	return json_string_new_length((uint32_t)sdslen(str), str);
}

// flags reader and writer
static INLINE json_value *otfcc_dump_flags(int flags, const char *labels[]) {
	json_value *v = json_object_new(0);
	for (uint16_t j = 0; labels[j]; j++)
		if (flags & (1 << j)) { json_object_push(v, labels[j], json_boolean_new(true)); }
	return v;
}
static INLINE uint32_t otfcc_parse_flags(const json_value *v, const char *labels[]) {
	if (!v) return 0;
	if (v->type == json_integer) {
		return (uint32_t)v->u.integer;
	} else if (v->type == json_double) {
		return (uint32_t)v->u.dbl;
	} else if (v->type == json_object) {
		uint32_t flags = 0;
		for (uint16_t j = 0; labels[j]; j++) {
			if (json_obj_getbool(v, labels[j])) { flags |= (1 << j); }
		}
		return flags;
	} else {
		return 0;
	}
}

static INLINE json_value *preserialize(MOVE json_value *x) {
#ifdef CARYLL_USE_PRE_SERIALIZED
	json_serialize_opts opts = {.mode = json_serialize_mode_packed};
	size_t preserialize_len = json_measure_ex(x, opts);
	char *buf = (char *)malloc(preserialize_len);
	json_serialize_ex(buf, x, opts);
	json_builder_free(x);

	json_value *xx = json_string_new_nocopy((uint32_t)(preserialize_len - 1), buf);
	xx->type = json_pre_serialized;
	return xx;
#else
	return x;
#endif
}

#endif
