#include "support/util.h"
#include "otfcc/table/otl/coverage.h"

static INLINE void disposeCoverage(MOVE otl_Coverage *coverage) {
	for (glyphid_t j = 0; j < coverage->numGlyphs; j++) {
		Handle.dispose(&coverage->glyphs[j]);
	}
	FREE(coverage->glyphs);
}
caryll_standardRefTypeFn(otl_Coverage, disposeCoverage);

static void growCoverage(otl_Coverage *coverage, uint32_t n) {
	if (!n) return;
	if (n > coverage->capacity) {
		if (!coverage->capacity) coverage->capacity = 0x10;
		while (n > coverage->capacity)
			coverage->capacity += (coverage->capacity >> 1) & 0xFFFFFF;
		RESIZE(coverage->glyphs, coverage->capacity);
	}
}
static void clearCoverage(otl_Coverage *coverage, uint32_t n) {
	if (!coverage || !coverage->glyphs) return;
	for (glyphid_t j = 0; j < coverage->numGlyphs; j++) {
		Handle.dispose(&coverage->glyphs[j]);
	}
	growCoverage(coverage, n);
	coverage->numGlyphs = n;
}

typedef struct {
	int gid;
	int covIndex;
	UT_hash_handle hh;
} coverage_entry;

static int by_covIndex(coverage_entry *a, coverage_entry *b) {
	return a->covIndex - b->covIndex;
}

static void pushToCoverage(otl_Coverage *coverage, MOVE otfcc_GlyphHandle h) {
	coverage->numGlyphs += 1;
	growCoverage(coverage, coverage->numGlyphs);
	coverage->glyphs[coverage->numGlyphs - 1] = h;
}

static otl_Coverage *readCoverage(const uint8_t *data, uint32_t tableLength, uint32_t offset) {
	otl_Coverage *coverage = otl_iCoverage.create();
	if (tableLength < offset + 4) return coverage;
	uint16_t format = read_16u(data + offset);
	switch (format) {
		case 1: {
			uint16_t glyphCount = read_16u(data + offset + 2);
			if (tableLength < offset + 4 + glyphCount * 2) return coverage;
			coverage_entry *hash = NULL;
			for (uint16_t j = 0; j < glyphCount; j++) {
				coverage_entry *item = NULL;
				int gid = read_16u(data + offset + 4 + j * 2);
				HASH_FIND_INT(hash, &gid, item);
				if (!item) {
					NEW(item);
					item->gid = gid;
					item->covIndex = j;
					HASH_ADD_INT(hash, gid, item);
				}
			}
			HASH_SORT(hash, by_covIndex);
			coverage_entry *e, *tmp;
			HASH_ITER(hh, hash, e, tmp) {
				pushToCoverage(coverage, Handle.fromIndex(e->gid));
				HASH_DEL(hash, e);
				FREE(e);
			}
			break;
		}
		case 2: {
			uint16_t rangeCount = read_16u(data + offset + 2);
			if (tableLength < offset + 4 + rangeCount * 6) return coverage;
			coverage_entry *hash = NULL;
			for (uint16_t j = 0; j < rangeCount; j++) {
				uint16_t start = read_16u(data + offset + 4 + 6 * j);
				uint16_t end = read_16u(data + offset + 4 + 6 * j + 2);
				uint16_t startCoverageIndex = read_16u(data + offset + 4 + 6 * j + 4);
				for (int k = start; k <= end; k++) {
					coverage_entry *item = NULL;
					HASH_FIND_INT(hash, &k, item);
					if (!item) {
						NEW(item);
						item->gid = k;
						item->covIndex = startCoverageIndex + k;
						HASH_ADD_INT(hash, gid, item);
					}
				}
			}
			HASH_SORT(hash, by_covIndex);
			coverage_entry *e, *tmp;
			HASH_ITER(hh, hash, e, tmp) {
				pushToCoverage(coverage, Handle.fromIndex(e->gid));
				HASH_DEL(hash, e);
				FREE(e);
			}
			break;
		}
		default:
			break;
	}
	return coverage;
}

static json_value *dumpCoverage(const otl_Coverage *coverage) {
	json_value *a = json_array_new(coverage->numGlyphs);
	for (glyphid_t j = 0; j < coverage->numGlyphs; j++) {
		json_array_push(a, json_string_new(coverage->glyphs[j].name));
	}
	return preserialize(a);
}

static otl_Coverage *parseCoverage(const json_value *cov) {
	otl_Coverage *c = otl_iCoverage.create();
	if (!cov || cov->type != json_array) return c;

	for (glyphid_t j = 0; j < cov->u.array.length; j++) {
		if (cov->u.array.values[j]->type == json_string) {
			pushToCoverage(c,
			               Handle.fromName(sdsnewlen(cov->u.array.values[j]->u.string.ptr,
			                                         cov->u.array.values[j]->u.string.length)));
		}
	}
	return c;
}

static int by_gid(const void *a, const void *b) {
	return *((glyphid_t *)a) - *((glyphid_t *)b);
}

static caryll_Buffer *buildCoverageFormat(const otl_Coverage *coverage, uint16_t format) {
	// sort the gids in coverage
	if (!coverage->numGlyphs) {
		caryll_Buffer *buf = bufnew();
		bufwrite16b(buf, 2);
		bufwrite16b(buf, 0);
		return buf;
	}
	glyphid_t *r;
	NEW(r, coverage->numGlyphs);
	glyphid_t jj = 0;
	for (glyphid_t j = 0; j < coverage->numGlyphs; j++) {
		r[jj] = coverage->glyphs[j].index;
		jj++;
	}
	qsort(r, jj, sizeof(glyphid_t), by_gid);

	caryll_Buffer *format1 = bufnew();
	bufwrite16b(format1, 1);
	bufwrite16b(format1, jj);
	for (glyphid_t j = 0; j < jj; j++) {
		bufwrite16b(format1, r[j]);
	}
	if (jj < 2) {
		FREE(r);
		return format1;
	}

	caryll_Buffer *format2 = bufnew();
	bufwrite16b(format2, 2);
	caryll_Buffer *ranges = bufnew();
	glyphid_t startGID = r[0];
	glyphid_t endGID = startGID;
	glyphid_t lastGID = startGID;
	glyphid_t nRanges = 0;
	for (glyphid_t j = 1; j < jj; j++) {
		glyphid_t current = r[j];
		if (current <= lastGID) continue;
		if (current == endGID + 1) {
			endGID = current;
		} else {
			bufwrite16b(ranges, startGID);
			bufwrite16b(ranges, endGID);
			bufwrite16b(ranges, j + startGID - endGID - 1);
			nRanges += 1;
			startGID = endGID = current;
		}
		lastGID = current;
	}
	bufwrite16b(ranges, startGID);
	bufwrite16b(ranges, endGID);
	bufwrite16b(ranges, jj + startGID - endGID - 1);
	nRanges += 1;
	bufwrite16b(format2, nRanges);
	bufwrite_bufdel(format2, ranges);

	if (format == 1) {
		buffree(format2);
		FREE(r);
		return format1;
	} else if (format == 2) {
		buffree(format1);
		FREE(r);
		return format2;
	} else {
		if (buflen(format1) < buflen(format2)) {
			buffree(format2);
			FREE(r);
			return format1;
		} else {
			buffree(format1);
			FREE(r);
			return format2;
		}
	}
}

static caryll_Buffer *buildCoverage(const otl_Coverage *coverage) {
	return buildCoverageFormat(coverage, 0);
}

static int byHandleGID(const void *a, const void *b) {
	return ((glyph_handle *)a)->index - ((glyph_handle *)b)->index;
}

static void shrinkCoverage(otl_Coverage *coverage, bool dosort) {
	if (!coverage) return;
	glyphid_t k = 0;
	for (glyphid_t j = 0; j < coverage->numGlyphs; j++) {
		if (coverage->glyphs[j].name) {
			coverage->glyphs[k++] = coverage->glyphs[j];
		} else {
			Handle.dispose(&coverage->glyphs[j]);
		}
	}
	if (dosort) {
		qsort(coverage->glyphs, k, sizeof(glyph_handle), byHandleGID);
		glyphid_t skip = 0;
		for (glyphid_t rear = 1; rear < k; rear++) {
			if (coverage->glyphs[rear].index == coverage->glyphs[rear - skip - 1].index) {
				Handle.dispose(&coverage->glyphs[rear]);
				skip += 1;
			} else {
				coverage->glyphs[rear - skip] = coverage->glyphs[rear];
			}
		}
		k -= skip;
	}
	coverage->numGlyphs = k;
}

const struct __otfcc_ICoverage otl_iCoverage = {
    caryll_standardRefTypeMethods(otl_Coverage),
    .clear = clearCoverage,
    .read = readCoverage,
    .dump = dumpCoverage,
    .parse = parseCoverage,
    .build = buildCoverage,
	.buildFormat = buildCoverageFormat,
    .shrink = shrinkCoverage,
    .push = pushToCoverage,
};
