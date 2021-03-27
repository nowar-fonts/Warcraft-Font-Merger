#include "support/util.h"
#include "otfcc/table/otl/classdef.h"

static INLINE void disposeClassDef(otl_ClassDef *cd) {
	if (cd->glyphs) {
		for (glyphid_t j = 0; j < cd->numGlyphs; j++) {
			Handle.dispose(&cd->glyphs[j]);
		}
		FREE(cd->glyphs);
	}
	FREE(cd->classes);
}
caryll_standardRefTypeFn(otl_ClassDef, disposeClassDef);

static void growClassdef(otl_ClassDef *cd, uint32_t n) {
	if (!n) return;
	if (n > cd->capacity) {
		if (!cd->capacity) cd->capacity = 0x10;
		while (n > cd->capacity)
			cd->capacity += (cd->capacity >> 1) & 0xFFFFFF;
		RESIZE(cd->glyphs, cd->capacity);
		RESIZE(cd->classes, cd->capacity);
	}
}
static void pushClassDef(otl_ClassDef *cd, MOVE otfcc_GlyphHandle h, glyphclass_t cls) {
	cd->numGlyphs += 1;
	growClassdef(cd, cd->numGlyphs);
	cd->glyphs[cd->numGlyphs - 1] = h;
	cd->classes[cd->numGlyphs - 1] = cls;
	if (cls > cd->maxclass) cd->maxclass = cls;
}

typedef struct {
	int gid;
	int covIndex;
	UT_hash_handle hh;
} coverage_entry;

static int by_covIndex(coverage_entry *a, coverage_entry *b) {
	return a->covIndex - b->covIndex;
}

static otl_ClassDef *readClassDef(const uint8_t *data, uint32_t tableLength, uint32_t offset) {
	otl_ClassDef *cd = otl_iClassDef.create();
	if (tableLength < offset + 4) return cd;
	uint16_t format = read_16u(data + offset);
	if (format == 1 && tableLength >= offset + 6) {
		glyphid_t startGID = read_16u(data + offset + 2);
		glyphid_t count = read_16u(data + offset + 4);
		if (count && tableLength >= offset + 6 + count * 2) {
			for (glyphid_t j = 0; j < count; j++) {
				pushClassDef(cd, Handle.fromIndex(startGID + j), read_16u(data + offset + 6 + j * 2));
			}
			return cd;
		}
	} else if (format == 2) {
		// The ranges may overlap.
		// Use hashtable.
		uint16_t rangeCount = read_16u(data + offset + 2);
		if (tableLength < offset + 4 + rangeCount * 6) return cd;
		coverage_entry *hash = NULL;
		for (uint16_t j = 0; j < rangeCount; j++) {
			uint16_t start = read_16u(data + offset + 4 + 6 * j);
			uint16_t end = read_16u(data + offset + 4 + 6 * j + 2);
			uint16_t cls = read_16u(data + offset + 4 + 6 * j + 4);
			for (int k = start; k <= end; k++) {
				coverage_entry *item = NULL;
				HASH_FIND_INT(hash, &k, item);
				if (!item) {
					NEW(item);
					item->gid = k;
					item->covIndex = cls;
					HASH_ADD_INT(hash, gid, item);
				}
			}
		}
		HASH_SORT(hash, by_covIndex);
		coverage_entry *e, *tmp;
		HASH_ITER(hh, hash, e, tmp) {
			pushClassDef(cd, Handle.fromIndex(e->gid), e->covIndex);
			HASH_DEL(hash, e);
			FREE(e);
		}
		return cd;
	}
	return cd;
}

static otl_ClassDef *expandClassDef(otl_Coverage *cov, otl_ClassDef *ocd) {
	otl_ClassDef *cd = otl_iClassDef.create();
	coverage_entry *hash = NULL;
	for (glyphid_t j = 0; j < ocd->numGlyphs; j++) {
		int gid = ocd->glyphs[j].index;
		int cid = ocd->classes[j];
		coverage_entry *item = NULL;
		HASH_FIND_INT(hash, &gid, item);
		if (!item) {
			NEW(item);
			item->gid = gid;
			item->covIndex = cid;
			HASH_ADD_INT(hash, gid, item);
		}
	}
	for (glyphid_t j = 0; j < cov->numGlyphs; j++) {
		int gid = cov->glyphs[j].index;
		coverage_entry *item = NULL;
		HASH_FIND_INT(hash, &gid, item);
		if (!item) {
			NEW(item);
			item->gid = gid;
			item->covIndex = 0;
			HASH_ADD_INT(hash, gid, item);
		}
	}
	coverage_entry *e, *tmp;
	HASH_ITER(hh, hash, e, tmp) {
		pushClassDef(cd, Handle.fromIndex(e->gid), e->covIndex);
		HASH_DEL(hash, e);
		FREE(e);
	}
	otl_iClassDef.free(ocd);
	return cd;
}

static json_value *dumpClassDef(const otl_ClassDef *cd) {
	json_value *a = json_object_new(cd->numGlyphs);
	for (glyphid_t j = 0; j < cd->numGlyphs; j++) {
		json_object_push(a, cd->glyphs[j].name, json_integer_new(cd->classes[j]));
	}
	return preserialize(a);
}

static otl_ClassDef *parseClassDef(const json_value *_cd) {
	if (!_cd || _cd->type != json_object) return NULL;
	otl_ClassDef *cd = otl_iClassDef.create();
	for (glyphid_t j = 0; j < _cd->u.object.length; j++) {
		glyph_handle h = Handle.fromName(sdsnewlen(_cd->u.object.values[j].name, _cd->u.object.values[j].name_length));
		json_value *_cid = _cd->u.object.values[j].value;
		glyphclass_t cls = 0;
		if (_cid->type == json_integer) {
			cls = _cid->u.integer;
		} else if (_cid->type == json_double) {
			cls = _cid->u.dbl;
		}
		pushClassDef(cd, h, cls);
	}
	return cd;
}

typedef struct {
	glyphid_t gid;
	glyphclass_t cid;
} classdef_sortrecord;

static int by_gid(const void *a, const void *b) {
	return ((classdef_sortrecord *)a)->gid - ((classdef_sortrecord *)b)->gid;
}
static caryll_Buffer *buildClassDef(const otl_ClassDef *cd) {
	caryll_Buffer *buf = bufnew();
	bufwrite16b(buf, 2);
	if (!cd->numGlyphs) { // no glyphs, return a blank classdef
		bufwrite16b(buf, 0);
		return buf;
	}

	classdef_sortrecord *r;
	NEW(r, cd->numGlyphs);
	glyphid_t jj = 0;
	for (glyphid_t j = 0; j < cd->numGlyphs; j++) {
		if (cd->classes[j]) {
			r[jj].gid = cd->glyphs[j].index;
			r[jj].cid = cd->classes[j];
			jj++;
		}
	}
	if (!jj) { // The classdef has only class 0
		FREE(r);
		bufwrite16b(buf, 0);
		return buf;
	}
	qsort(r, jj, sizeof(classdef_sortrecord), by_gid);

	glyphid_t startGID = r[0].gid;
	glyphid_t endGID = startGID;
	glyphclass_t lastClass = r[0].cid;
	glyphid_t nRanges = 0;
	glyphid_t lastGID = startGID;
	caryll_Buffer *ranges = bufnew();
	for (glyphid_t j = 1; j < jj; j++) {
		glyphid_t current = r[j].gid;
		if (current <= lastGID) continue;
		if (current == endGID + 1 && r[j].cid == lastClass) {
			endGID = current;
		} else {
			bufwrite16b(ranges, startGID);
			bufwrite16b(ranges, endGID);
			bufwrite16b(ranges, lastClass);
			nRanges += 1;
			startGID = endGID = current;
			lastClass = r[j].cid;
		}
		lastGID = current;
	}
	bufwrite16b(ranges, startGID);
	bufwrite16b(ranges, endGID);
	bufwrite16b(ranges, lastClass);
	nRanges += 1;
	bufwrite16b(buf, nRanges);
	bufwrite_bufdel(buf, ranges);
	FREE(r);
	return buf;
}

static void shrinkClassDef(otl_ClassDef *cd) {
	glyphid_t k = 0;
	for (glyphid_t j = 0; j < cd->numGlyphs; j++) {
		if (cd->glyphs[j].name) {
			cd->glyphs[k] = cd->glyphs[j];
			cd->classes[k] = cd->classes[j];
			k++;
		} else {
			Handle.dispose(&cd->glyphs[j]);
		}
	}
	cd->numGlyphs = k;
}

const struct __otfcc_IClassDef otl_iClassDef = {
    caryll_standardRefTypeMethods(otl_ClassDef),
    .read = readClassDef,
    .expand = expandClassDef,
    .dump = dumpClassDef,
    .parse = parseClassDef,
    .build = buildClassDef,
    .shrink = shrinkClassDef,
    .push = pushClassDef,
};
