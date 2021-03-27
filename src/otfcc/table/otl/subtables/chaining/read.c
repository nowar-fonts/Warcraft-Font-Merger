#include "../chaining.h"
#include "common.h"

typedef struct {
	otl_ClassDef *bc;
	otl_ClassDef *ic;
	otl_ClassDef *fc;
} classdefs;

otl_Coverage *singleCoverage(font_file_pointer data, uint32_t tableLength, uint16_t gid,
                             uint32_t _offset, uint16_t kind, const glyphid_t maxGlyphs,
                             void *userdata) {
	otl_Coverage *cov;
	NEW(cov);
	cov->numGlyphs = 1;
	NEW(cov->glyphs);
	cov->glyphs[0] = Handle.fromIndex((glyphid_t)gid);
	return cov;
}
otl_Coverage *classCoverage(font_file_pointer data, uint32_t tableLength, uint16_t cls,
                            uint32_t _offset, uint16_t kind, const glyphid_t maxGlyphs,
                            void *_classdefs) {
	classdefs *defs = (classdefs *)_classdefs;
	otl_ClassDef *cd = (kind == 1 ? defs->bc : kind == 2 ? defs->ic : defs->fc);
	otl_Coverage *cov;
	NEW(cov);
	cov->numGlyphs = 0;
	cov->glyphs = NULL;
	glyphid_t count = 0;
	if (cls == 0) {
		for (glyphid_t k = 0; k < maxGlyphs; k++) {
			bool found = false;
			for (glyphid_t j = 0; j < cd->numGlyphs; j++) {
				if (cd->classes[j] > 0 && cd->glyphs[j].index == k) {
					found = true;
					break;
				}
			}
			if (!found) count += 1;
		}
	} else {
		for (glyphid_t j = 0; j < cd->numGlyphs; j++) {
			if (cd->classes[j] == cls) count++;
		}
	}
	if (!count) return cov;

	cov->numGlyphs = count;
	NEW(cov->glyphs, count);
	glyphid_t jj = 0;
	if (cls == 0) {
		for (glyphid_t k = 0; k < maxGlyphs; k++) {
			bool found = false;
			for (glyphid_t j = 0; j < cd->numGlyphs; j++) {
				if (cd->classes[j] > 0 && cd->glyphs[j].index == k) {
					found = true;
					break;
				}
			}
			if (!found) cov->glyphs[jj++] = Handle.fromIndex(k);
		}
	} else {
		for (glyphid_t j = 0; j < cd->numGlyphs; j++) {
			if (cd->classes[j] == cls) { cov->glyphs[jj++] = Handle.dup(cd->glyphs[j]); }
		}
	}
	return cov;
}
otl_Coverage *format3Coverage(font_file_pointer data, uint32_t tableLength, uint16_t shift,
                              uint32_t _offset, uint16_t kind, const glyphid_t maxGlyphs,
                              void *userdata) {
	return Coverage.read(data, tableLength, _offset + shift - 2);
}

typedef otl_Coverage *(*CoverageReaderHandler)(font_file_pointer, uint32_t, uint16_t, uint32_t,
                                               uint16_t, glyphid_t, void *);

otl_ChainingRule *GeneralReadContextualRule(font_file_pointer data, uint32_t tableLength,
                                            uint32_t offset, uint16_t startGID, bool minusOne,
                                            CoverageReaderHandler fn, const glyphid_t maxGlyphs,
                                            void *userdata) {
	otl_ChainingRule *rule;
	NEW(rule);
	rule->match = NULL;
	rule->apply = NULL;

	uint16_t minusOneQ = (minusOne ? 1 : 0);

	checkLength(offset + 4);
	uint16_t nInput = read_16u(data + offset);
	uint16_t nApply = read_16u(data + offset + 2);
	checkLength(offset + 4 + 2 * nInput + 4 * nApply);

	rule->matchCount = nInput;
	rule->inputBegins = 0;
	rule->inputEnds = nInput;

	NEW(rule->match, rule->matchCount);
	uint16_t jj = 0;
	if (minusOne) {
		rule->match[jj++] = fn(data, tableLength, startGID, offset, 2, maxGlyphs, userdata);
	}
	for (uint16_t j = 0; j < nInput - minusOneQ; j++) {
		uint32_t gid = read_16u(data + offset + 4 + j * 2);
		rule->match[jj++] = fn(data, tableLength, gid, offset, 2, maxGlyphs, userdata);
	}
	rule->applyCount = nApply;
	NEW(rule->apply, rule->applyCount);
	for (tableid_t j = 0; j < nApply; j++) {
		rule->apply[j].index =
		    rule->inputBegins +
		    read_16u(data + offset + 4 + 2 * (rule->matchCount - minusOneQ) + j * 4);
		rule->apply[j].lookup = Handle.fromIndex(
		    read_16u(data + offset + 4 + 2 * (rule->matchCount - minusOneQ) + j * 4 + 2));
	}
	reverseBacktracks(rule);
	return rule;

FAIL:
	DELETE(deleteRule, rule);
	return NULL;
}
static subtable_chaining *readContextualFormat1(subtable_chaining *subtable,
                                                const font_file_pointer data, uint32_t tableLength,
                                                uint32_t offset, const glyphid_t maxGlyphs) {
	// Contextual Substitution Subtable, Simple.
	checkLength(offset + 6);

	uint16_t covOffset = offset + read_16u(data + offset + 2);
	otl_Coverage *firstCoverage = Coverage.read(data, tableLength, covOffset);

	tableid_t chainSubRuleSetCount = read_16u(data + offset + 4);
	if (chainSubRuleSetCount != firstCoverage->numGlyphs) goto FAIL;
	checkLength(offset + 6 + 2 * chainSubRuleSetCount);

	tableid_t totalRules = 0;
	for (tableid_t j = 0; j < chainSubRuleSetCount; j++) {
		uint32_t srsOffset = offset + read_16u(data + offset + 6 + j * 2);
		checkLength(srsOffset + 2);
		totalRules += read_16u(data + srsOffset);
		checkLength(srsOffset + 2 + 2 * read_16u(data + srsOffset));
	}
	subtable->rulesCount = totalRules;
	NEW(subtable->rules, totalRules);

	tableid_t jj = 0;
	for (tableid_t j = 0; j < chainSubRuleSetCount; j++) {
		uint32_t srsOffset = offset + read_16u(data + offset + 6 + j * 2);
		tableid_t srsCount = read_16u(data + srsOffset);
		for (tableid_t k = 0; k < srsCount; k++) {
			uint32_t srOffset = srsOffset + read_16u(data + srsOffset + 2 + k * 2);
			subtable->rules[jj] = GeneralReadContextualRule(data, tableLength, srOffset,
			                                                firstCoverage->glyphs[j].index, true,
			                                                singleCoverage, maxGlyphs, NULL);
			jj += 1;
		}
	}

	Coverage.free(firstCoverage);
	return subtable;
FAIL:
	iSubtable_chaining.free(subtable);
	return NULL;
}
static subtable_chaining *readContextualFormat2(subtable_chaining *subtable,
                                                const font_file_pointer data, uint32_t tableLength,
                                                uint32_t offset, const glyphid_t maxGlyphs) {
	// Contextual Substitution Subtable, Class based.
	checkLength(offset + 8);

	classdefs *cds;
	NEW(cds);
	cds->bc = NULL;
	cds->ic = ClassDef.read(data, tableLength, offset + read_16u(data + offset + 4));
	cds->fc = NULL;

	tableid_t chainSubClassSetCnt = read_16u(data + offset + 6);
	checkLength(offset + 12 + 2 * chainSubClassSetCnt);

	tableid_t totalRules = 0;
	for (tableid_t j = 0; j < chainSubClassSetCnt; j++) {
		uint32_t srcOffset = read_16u(data + offset + 8 + j * 2);
		if (srcOffset) { totalRules += read_16u(data + offset + srcOffset); }
	}
	subtable->rulesCount = totalRules;
	NEW(subtable->rules, totalRules);

	tableid_t jj = 0;
	for (tableid_t j = 0; j < chainSubClassSetCnt; j++) {
		uint32_t srcOffset = read_16u(data + offset + 8 + j * 2);
		if (srcOffset) {
			tableid_t srsCount = read_16u(data + offset + srcOffset);
			for (tableid_t k = 0; k < srsCount; k++) {
				uint32_t srOffset =
				    offset + srcOffset + read_16u(data + offset + srcOffset + 2 + k * 2);
				subtable->rules[jj] = GeneralReadContextualRule(
				    data, tableLength, srOffset, j, true, classCoverage, maxGlyphs, cds);
				jj += 1;
			}
		}
	}
	if (cds) {
		if (cds->bc) ClassDef.free(cds->bc);
		if (cds->ic) ClassDef.free(cds->ic);
		if (cds->fc) ClassDef.free(cds->fc);
		FREE(cds);
	}
	return subtable;
FAIL:
	iSubtable_chaining.free(subtable);
	return NULL;
}
otl_Subtable *otl_read_contextual(const font_file_pointer data, uint32_t tableLength,
                                  uint32_t offset, const glyphid_t maxGlyphs,
                                  const otfcc_Options *options) {
	uint16_t format = 0;
	subtable_chaining *subtable = iSubtable_chaining.create();
	subtable->type = otl_chaining_poly;

	checkLength(offset + 2);
	format = read_16u(data + offset);
	if (format == 1) {
		return (otl_Subtable *)readContextualFormat1(subtable, data, tableLength, offset,
		                                             maxGlyphs);
	} else if (format == 2) {
		return (otl_Subtable *)readContextualFormat2(subtable, data, tableLength, offset,
		                                             maxGlyphs);
	} else if (format == 3) {
		// Contextual Substitution Subtable, Coverage based.
		subtable->rulesCount = 1;
		NEW(subtable->rules, 1);
		subtable->rules[0] = GeneralReadContextualRule(data, tableLength, offset + 2, 0, false,
		                                               format3Coverage, maxGlyphs, NULL);
		return (otl_Subtable *)subtable;
	}
FAIL:
	logWarning("Unsupported format %d.\n", format);
	iSubtable_chaining.free(subtable);
	return NULL;
}

otl_ChainingRule *GeneralReadChainingRule(font_file_pointer data, uint32_t tableLength,
                                          uint32_t offset, uint16_t startGID, bool minusOne,
                                          CoverageReaderHandler fn, const glyphid_t maxGlyphs,
                                          void *userdata) {
	otl_ChainingRule *rule;
	NEW(rule);
	rule->match = NULL;
	rule->apply = NULL;

	uint16_t minusOneQ = (minusOne ? 1 : 0);

	checkLength(offset + 8);
	tableid_t nBack = read_16u(data + offset);
	checkLength(offset + 2 + 2 * nBack + 2);
	tableid_t nInput = read_16u(data + offset + 2 + 2 * nBack);
	checkLength(offset + 4 + 2 * (nBack + nInput - minusOneQ) + 2);
	tableid_t nLookaround = read_16u(data + offset + 4 + 2 * (nBack + nInput - minusOneQ));
	checkLength(offset + 6 + 2 * (nBack + nInput - minusOneQ + nLookaround) + 2);
	tableid_t nApply = read_16u(data + offset + 6 + 2 * (nBack + nInput - minusOneQ + nLookaround));
	checkLength(offset + 8 + 2 * (nBack + nInput - minusOneQ + nLookaround) + nApply * 4);

	rule->matchCount = nBack + nInput + nLookaround;
	rule->inputBegins = nBack;
	rule->inputEnds = nBack + nInput;

	NEW(rule->match, rule->matchCount);
	tableid_t jj = 0;
	for (tableid_t j = 0; j < nBack; j++) {
		uint32_t gid = read_16u(data + offset + 2 + j * 2);
		rule->match[jj++] = fn(data, tableLength, gid, offset, 1, maxGlyphs, userdata);
	}
	if (minusOne) {
		rule->match[jj++] = fn(data, tableLength, startGID, offset, 2, maxGlyphs, userdata);
	}
	for (tableid_t j = 0; j < nInput - minusOneQ; j++) {
		uint32_t gid = read_16u(data + offset + 4 + 2 * rule->inputBegins + j * 2);
		rule->match[jj++] = fn(data, tableLength, gid, offset, 2, maxGlyphs, userdata);
	}
	for (tableid_t j = 0; j < nLookaround; j++) {
		uint32_t gid = read_16u(data + offset + 6 + 2 * (rule->inputEnds - minusOneQ) + j * 2);
		rule->match[jj++] = fn(data, tableLength, gid, offset, 3, maxGlyphs, userdata);
	}
	rule->applyCount = nApply;
	NEW(rule->apply, rule->applyCount);
	for (tableid_t j = 0; j < nApply; j++) {
		rule->apply[j].index =
		    rule->inputBegins +
		    read_16u(data + offset + 8 + 2 * (rule->matchCount - minusOneQ) + j * 4);
		rule->apply[j].lookup = Handle.fromIndex(
		    read_16u(data + offset + 8 + 2 * (rule->matchCount - minusOneQ) + j * 4 + 2));
	}
	reverseBacktracks(rule);
	return rule;

FAIL:
	DELETE(deleteRule, rule);
	return NULL;
}
static subtable_chaining *readChainingFormat1(subtable_chaining *subtable,
                                              const font_file_pointer data, uint32_t tableLength,
                                              uint32_t offset, const glyphid_t maxGlyphs) {
	// Contextual Substitution Subtable, Simple.
	checkLength(offset + 6);

	uint16_t covOffset = offset + read_16u(data + offset + 2);
	otl_Coverage *firstCoverage = Coverage.read(data, tableLength, covOffset);

	tableid_t chainSubRuleSetCount = read_16u(data + offset + 4);
	if (chainSubRuleSetCount != firstCoverage->numGlyphs) goto FAIL;
	checkLength(offset + 6 + 2 * chainSubRuleSetCount);

	tableid_t totalRules = 0;
	for (tableid_t j = 0; j < chainSubRuleSetCount; j++) {
		uint32_t srsOffset = offset + read_16u(data + offset + 6 + j * 2);
		checkLength(srsOffset + 2);
		totalRules += read_16u(data + srsOffset);
		checkLength(srsOffset + 2 + 2 * read_16u(data + srsOffset));
	}
	subtable->rulesCount = totalRules;
	NEW(subtable->rules, totalRules);

	tableid_t jj = 0;
	for (tableid_t j = 0; j < chainSubRuleSetCount; j++) {
		uint32_t srsOffset = offset + read_16u(data + offset + 6 + j * 2);
		tableid_t srsCount = read_16u(data + srsOffset);
		for (tableid_t k = 0; k < srsCount; k++) {
			uint32_t srOffset = srsOffset + read_16u(data + srsOffset + 2 + k * 2);
			subtable->rules[jj] =
			    GeneralReadChainingRule(data, tableLength, srOffset, firstCoverage->glyphs[j].index,
			                            true, singleCoverage, maxGlyphs, NULL);
			jj += 1;
		}
	}

	Coverage.free(firstCoverage);
	return subtable;
FAIL:
	iSubtable_chaining.free(subtable);
	return NULL;
}
static subtable_chaining *readChainingFormat2(subtable_chaining *subtable,
                                              const font_file_pointer data, uint32_t tableLength,
                                              uint32_t offset, const glyphid_t maxGlyphs) {
	// Chaining Contextual Substitution Subtable, Class based.
	checkLength(offset + 12);

	classdefs *cds;
	NEW(cds);
	cds->bc = ClassDef.read(data, tableLength, offset + read_16u(data + offset + 4));
	cds->ic = ClassDef.read(data, tableLength, offset + read_16u(data + offset + 6));
	cds->fc = ClassDef.read(data, tableLength, offset + read_16u(data + offset + 8));

	tableid_t chainSubClassSetCnt = read_16u(data + offset + 10);
	checkLength(offset + 12 + 2 * chainSubClassSetCnt);

	tableid_t totalRules = 0;
	for (tableid_t j = 0; j < chainSubClassSetCnt; j++) {
		uint32_t srcOffset = read_16u(data + offset + 12 + j * 2);
		if (srcOffset) { totalRules += read_16u(data + offset + srcOffset); }
	}
	subtable->rulesCount = totalRules;
	NEW(subtable->rules, totalRules);

	tableid_t jj = 0;
	for (tableid_t j = 0; j < chainSubClassSetCnt; j++) {
		uint32_t srcOffset = read_16u(data + offset + 12 + j * 2);
		if (srcOffset) {
			tableid_t srsCount = read_16u(data + offset + srcOffset);
			for (tableid_t k = 0; k < srsCount; k++) {
				uint32_t dsrOffset = read_16u(data + offset + srcOffset + 2 + k * 2);
				uint32_t srOffset = offset + srcOffset + dsrOffset;
				subtable->rules[jj] = GeneralReadChainingRule(data, tableLength, srOffset, j, true,
				                                              classCoverage, maxGlyphs, cds);
				jj += 1;
			}
		}
	}

	if (cds) {
		if (cds->bc) ClassDef.free(cds->bc);
		if (cds->ic) ClassDef.free(cds->ic);
		if (cds->fc) ClassDef.free(cds->fc);
		FREE(cds);
	}
	return subtable;
FAIL:
	iSubtable_chaining.free(subtable);
	return NULL;
}
otl_Subtable *otl_read_chaining(const font_file_pointer data, uint32_t tableLength, uint32_t offset,
                                const glyphid_t maxGlyphs, const otfcc_Options *options) {
	uint16_t format = 0;
	subtable_chaining *subtable = iSubtable_chaining.create();
	subtable->type = otl_chaining_poly;

	checkLength(offset + 2);
	format = read_16u(data + offset);
	if (format == 1) {
		return (otl_Subtable *)readChainingFormat1(subtable, data, tableLength, offset, maxGlyphs);
	} else if (format == 2) {
		return (otl_Subtable *)readChainingFormat2(subtable, data, tableLength, offset, maxGlyphs);
	} else if (format == 3) {
		// Chaining Contextual Substitution Subtable, Coverage based.
		// This table has exactly one rule within it, and i love it.
		subtable->rulesCount = 1;
		NEW(subtable->rules, 1);
		subtable->rules[0] = GeneralReadChainingRule(data, tableLength, offset + 2, 0, false,
		                                             format3Coverage, maxGlyphs, NULL);
		return (otl_Subtable *)subtable;
	}
FAIL:
	logWarning("Unsupported format %d.\n", format);
	iSubtable_chaining.free(subtable);
	return NULL;
}
