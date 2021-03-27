#include "../chaining.h"
#include "common.h"

bool otfcc_chainingLookupIsContextualLookup(const otl_Lookup *lookup) {
	if (!(lookup->type == otl_type_gpos_chaining || lookup->type == otl_type_gsub_chaining))
		return false;
	bool isContextual = true;
	for (tableid_t j = 0; j < lookup->subtables.length; j++) {
		const subtable_chaining *subtable = &(lookup->subtables.items[j]->chaining);
		if (subtable->type == otl_chaining_classified) {
			for (tableid_t k = 0; k < subtable->rulesCount; k++) {
				otl_ChainingRule *rule = subtable->rules[k];
				tableid_t nBacktrack = rule->inputBegins;
				tableid_t nLookahead = rule->matchCount - rule->inputEnds;
				isContextual = isContextual && !nBacktrack && !nLookahead;
			}
		} else {
			otl_ChainingRule *rule = (otl_ChainingRule *)&(subtable->rule);
			tableid_t nBacktrack = rule->inputBegins;
			tableid_t nLookahead = rule->matchCount - rule->inputEnds;
			isContextual = isContextual && !nBacktrack && !nLookahead;
		}
	}
	return isContextual;
}

////// CHAINING

caryll_Buffer *otfcc_build_chaining_coverage(const otl_Subtable *_subtable) {
	const subtable_chaining *subtable = &(_subtable->chaining);
	otl_ChainingRule *rule = (otl_ChainingRule *)&(subtable->rule);
	tableid_t nBacktrack = rule->inputBegins;
	tableid_t nInput = rule->inputEnds - rule->inputBegins;
	tableid_t nLookahead = rule->matchCount - rule->inputEnds;
	tableid_t nSubst = rule->applyCount;
	reverseBacktracks(rule);

	bk_Block *root = bk_new_Block(b16, 3, // format
	                              bkover);

	bk_push(root, b16, nBacktrack, bkover);
	for (tableid_t j = 0; j < rule->inputBegins; j++) {
		bk_push(root, p16, bk_newBlockFromBuffer(Coverage.build(rule->match[j])), bkover);
	}
	bk_push(root, b16, nInput, bkover);
	for (tableid_t j = rule->inputBegins; j < rule->inputEnds; j++) {
		bk_push(root, p16, bk_newBlockFromBuffer(Coverage.build(rule->match[j])), bkover);
	}
	bk_push(root, b16, nLookahead, bkover);
	for (tableid_t j = rule->inputEnds; j < rule->matchCount; j++) {
		bk_push(root, p16, bk_newBlockFromBuffer(Coverage.build(rule->match[j])), bkover);
	}
	bk_push(root, b16, rule->applyCount, bkover);
	for (tableid_t j = 0; j < nSubst; j++) {
		bk_push(root, b16, rule->apply[j].index - nBacktrack, // position
		        b16, rule->apply[j].lookup.index,             // lookup
		        bkover);
	}

	return bk_build_Block(root);
}

caryll_Buffer *otfcc_build_chaining_classes(const otl_Subtable *_subtable) {
	const subtable_chaining *subtable = &(_subtable->chaining);

	otl_Coverage *coverage;
	NEW(coverage);
	coverage->numGlyphs = subtable->ic->numGlyphs;
	coverage->glyphs = subtable->ic->glyphs;

	bk_Block *root =
	    bk_new_Block(b16, 2,                                                   // format
	                 p16, bk_newBlockFromBuffer(Coverage.build(coverage)),     // coverage
	                 p16, bk_newBlockFromBuffer(ClassDef.build(subtable->bc)), // BacktrackClassDef
	                 p16, bk_newBlockFromBuffer(ClassDef.build(subtable->ic)), // InputClassDef
	                 p16, bk_newBlockFromBuffer(ClassDef.build(subtable->fc)), // LookaheadClassDef
	                 b16, subtable->ic->maxclass + 1, // ChainSubClassSetCnt
	                 bkover);

	glyphclass_t *rcpg;
	NEW(rcpg, subtable->ic->maxclass + 1);
	for (glyphclass_t j = 0; j <= subtable->ic->maxclass; j++) {
		rcpg[j] = 0;
	}
	for (tableid_t j = 0; j < subtable->rulesCount; j++) {
		tableid_t ib = subtable->rules[j]->inputBegins;
		tableid_t startClass = subtable->rules[j]->match[ib]->glyphs[0].index;
		if (startClass <= subtable->ic->maxclass) rcpg[startClass] += 1;
	}

	for (glyphclass_t j = 0; j <= subtable->ic->maxclass; j++) {
		if (rcpg[j]) {
			bk_Block *cset = bk_new_Block(b16, rcpg[j], // ChainSubClassRuleCnt
			                              bkover);
			for (tableid_t k = 0; k < subtable->rulesCount; k++) {
				otl_ChainingRule *rule = subtable->rules[k];
				glyphclass_t startClass = rule->match[rule->inputBegins]->glyphs[0].index;
				if (startClass != j) { continue; }
				reverseBacktracks(rule);
				tableid_t nBacktrack = rule->inputBegins;
				tableid_t nInput = rule->inputEnds - rule->inputBegins;
				tableid_t nLookahead = rule->matchCount - rule->inputEnds;
				tableid_t nSubst = rule->applyCount;
				bk_Block *r = bk_new_Block(bkover);
				bk_push(r, b16, nBacktrack, bkover);
				for (tableid_t m = 0; m < rule->inputBegins; m++) {
					bk_push(r, b16, rule->match[m]->glyphs[0].index, bkover);
				}
				bk_push(r, b16, nInput, bkover);
				for (tableid_t m = rule->inputBegins + 1; m < rule->inputEnds; m++) {
					bk_push(r, b16, rule->match[m]->glyphs[0].index, bkover);
				}
				bk_push(r, b16, nLookahead, bkover);
				for (tableid_t m = rule->inputEnds; m < rule->matchCount; m++) {
					bk_push(r, b16, rule->match[m]->glyphs[0].index, bkover);
				}
				bk_push(r, b16, nSubst, bkover);
				for (tableid_t m = 0; m < nSubst; m++) {
					bk_push(r, b16, rule->apply[m].index - nBacktrack, // position
					        b16, rule->apply[m].lookup.index,          // lookup index
					        bkover);
				}
				bk_push(cset, p16, r, bkover);
			}
			bk_push(root, p16, cset, bkover);
		} else {
			bk_push(root, p16, NULL, bkover);
		}
	}

	FREE(coverage);
	FREE(rcpg);
	return bk_build_Block(root);
}

caryll_Buffer *otfcc_build_chaining(const otl_Subtable *_subtable) {
	if (_subtable->chaining.type == otl_chaining_classified) {
		return otfcc_build_chaining_classes(_subtable);
	} else {
		return otfcc_build_chaining_coverage(_subtable);
	}
}

////// CONTEXTUAL

caryll_Buffer *otfcc_build_contextual_coverage(const otl_Subtable *_subtable) {
	const subtable_chaining *subtable = &(_subtable->chaining);
	otl_ChainingRule *rule = (otl_ChainingRule *)&(subtable->rule);
	tableid_t nInput = rule->inputEnds - rule->inputBegins;
	tableid_t nSubst = rule->applyCount;
	reverseBacktracks(rule);

	bk_Block *root = bk_new_Block(b16, 3, // format
	                              bkover);

	bk_push(root, b16, nInput, bkover);
	bk_push(root, b16, nSubst, bkover);
	for (tableid_t j = rule->inputBegins; j < rule->inputEnds; j++) {
		bk_push(root, p16, bk_newBlockFromBuffer(Coverage.build(rule->match[j])), bkover);
	}
	for (tableid_t j = 0; j < nSubst; j++) {
		bk_push(root, b16, rule->apply[j].index,  // position
		        b16, rule->apply[j].lookup.index, // lookup
		        bkover);
	}

	return bk_build_Block(root);
}

caryll_Buffer *otfcc_build_contextual_classes(const otl_Subtable *_subtable) {
	const subtable_chaining *subtable = &(_subtable->chaining);

	otl_Coverage *coverage;
	NEW(coverage);
	coverage->numGlyphs = subtable->ic->numGlyphs;
	coverage->glyphs = subtable->ic->glyphs;

	bk_Block *root =
	    bk_new_Block(b16, 2,                                                   // format
	                 p16, bk_newBlockFromBuffer(Coverage.build(coverage)),     // coverage
	                 p16, bk_newBlockFromBuffer(ClassDef.build(subtable->ic)), // InputClassDef
	                 b16, subtable->ic->maxclass + 1, // ChainSubClassSetCnt
	                 bkover);

	glyphclass_t *rcpg;
	NEW(rcpg, subtable->ic->maxclass + 1);
	for (glyphclass_t j = 0; j <= subtable->ic->maxclass; j++) {
		rcpg[j] = 0;
	}
	for (tableid_t j = 0; j < subtable->rulesCount; j++) {
		tableid_t ib = subtable->rules[j]->inputBegins;
		tableid_t startClass = subtable->rules[j]->match[ib]->glyphs[0].index;
		if (startClass <= subtable->ic->maxclass) rcpg[startClass] += 1;
	}

	for (glyphclass_t j = 0; j <= subtable->ic->maxclass; j++) {
		if (rcpg[j]) {
			bk_Block *cset = bk_new_Block(b16, rcpg[j], // ChainSubClassRuleCnt
			                              bkover);
			for (tableid_t k = 0; k < subtable->rulesCount; k++) {
				otl_ChainingRule *rule = subtable->rules[k];
				glyphclass_t startClass = rule->match[rule->inputBegins]->glyphs[0].index;
				if (startClass != j) { continue; }
				reverseBacktracks(rule);
				tableid_t nInput = rule->inputEnds - rule->inputBegins;
				tableid_t nSubst = rule->applyCount;
				bk_Block *r = bk_new_Block(bkover);

				bk_push(r, b16, nInput, bkover);
				bk_push(r, b16, nSubst, bkover);
				for (tableid_t m = rule->inputBegins + 1; m < rule->inputEnds; m++) {
					bk_push(r, b16, rule->match[m]->glyphs[0].index, bkover);
				}

				for (tableid_t m = 0; m < nSubst; m++) {
					bk_push(r, b16, rule->apply[m].index,     // position
					        b16, rule->apply[m].lookup.index, // lookup index
					        bkover);
				}
				bk_push(cset, p16, r, bkover);
			}
			bk_push(root, p16, cset, bkover);
		} else {
			bk_push(root, p16, NULL, bkover);
		}
	}

	FREE(coverage);
	FREE(rcpg);
	return bk_build_Block(root);
}

caryll_Buffer *otfcc_build_contextual(const otl_Subtable *_subtable) {
	if (_subtable->chaining.type == otl_chaining_classified) {
		return otfcc_build_contextual_classes(_subtable);
	} else {
		return otfcc_build_contextual_coverage(_subtable);
	}
}
