#include "../chaining.h"

// Chaining substitution classifier
// We will merge similar subtables.

typedef struct {
	int gid;
	sds gname;
	int cls;
	UT_hash_handle hh;
} classifier_hash;
static int by_gid_clsh(classifier_hash *a, classifier_hash *b) {
	return a->gid - b->gid;
}

static int classCompatible(classifier_hash **h, otl_Coverage *cov, int *past) {
	// checks whether a coverage is compatible to a class hash.
	classifier_hash *s = NULL;
	if (cov->numGlyphs == 0) return 1;
	int gid = cov->glyphs[0].index;
	// check pass
	HASH_FIND_INT(*h, &gid, s);
	if (s) {
		// the coverage has been defined into a class
		classifier_hash *ss, *tmp;
		for (glyphid_t j = 1; j < cov->numGlyphs; j++) {
			int gid = cov->glyphs[j].index;
			HASH_FIND_INT(*h, &gid, ss);
			if (!ss || ss->cls != s->cls) return 0;
		}
		// reverse check: all glyphs classified are there in the coverage
		classifier_hash *revh = NULL;
		for (glyphid_t j = 0; j < cov->numGlyphs; j++) {
			int gid = cov->glyphs[j].index;
			classifier_hash *rss = NULL;
			HASH_FIND_INT(revh, &gid, rss);
			if (!rss) {
				NEW(rss);
				rss->gid = gid;
				rss->gname = cov->glyphs[j].name;
				rss->cls = s->cls;
				HASH_ADD_INT(revh, gid, rss);
			}
		}

		bool allcheck = true;
		foreach_hash(ss, *h) if (ss->cls == s->cls) {
			int gid = ss->gid;
			classifier_hash *rss;
			HASH_FIND_INT(revh, &gid, rss);
			if (!rss) {
				allcheck = false;
				break;
			}
		}
		HASH_ITER(hh, revh, ss, tmp) {
			HASH_DEL(revh, ss);
			FREE(ss);
		}
		return allcheck ? s->cls : 0;
	} else {
		// the coverage is not defined into a class.
		classifier_hash *ss;
		for (glyphid_t j = 1; j < cov->numGlyphs; j++) {
			int gid = cov->glyphs[j].index;
			HASH_FIND_INT(*h, &gid, ss);
			if (ss) return 0;
		}
		for (glyphid_t j = 0; j < cov->numGlyphs; j++) {
			int gid = cov->glyphs[j].index;
			classifier_hash *s = NULL;
			HASH_FIND_INT(*h, &gid, s);
			if (!s) {
				NEW(s);
				s->gid = cov->glyphs[j].index;
				s->gname = cov->glyphs[j].name;
				s->cls = *past + 1;
				HASH_ADD_INT(*h, gid, s);
			}
		}
		*past += 1;
		return 1;
	}
}
static otl_ChainingRule *buildRule(otl_ChainingRule *rule, classifier_hash *hb, classifier_hash *hi,
                                   classifier_hash *hf) {
	otl_ChainingRule *newRule;
	NEW(newRule);
	newRule->matchCount = rule->matchCount;
	newRule->inputBegins = rule->inputBegins;
	newRule->inputEnds = rule->inputEnds;
	NEW(newRule->match, newRule->matchCount);
	for (tableid_t m = 0; m < rule->matchCount; m++) {
		NEW(newRule->match[m]);
		newRule->match[m]->numGlyphs = 1;
		NEW(newRule->match[m]->glyphs);
		if (rule->match[m]->numGlyphs > 0) {
			classifier_hash *h = (m < rule->inputBegins ? hb : m < rule->inputEnds ? hi : hf);
			classifier_hash *s;
			int gid = rule->match[m]->glyphs[0].index;
			HASH_FIND_INT(h, &gid, s);
			newRule->match[m]->glyphs[0] = Handle.fromIndex(s->cls);
		} else {
			newRule->match[m]->glyphs[0] = Handle.fromIndex(0);
		}
	}
	newRule->applyCount = rule->applyCount;
	NEW(newRule->apply, newRule->applyCount);
	for (tableid_t j = 0; j < rule->applyCount; j++) {
		newRule->apply[j].index = rule->apply[j].index;
		newRule->apply[j].lookup = Handle.dup(rule->apply[j].lookup);
	}
	return newRule;
}
static otl_ClassDef *toClass(classifier_hash **h) {
	otl_ClassDef *cd = ClassDef.create();
	classifier_hash *item;
	HASH_SORT(*h, by_gid_clsh);
	foreach_hash(item, *h) {
		ClassDef.push(cd, Handle.fromConsolidated(item->gid, item->gname), item->cls);
	}
	return cd;
}
tableid_t tryClassifyAround(const otl_Lookup *lookup, tableid_t j,
                            OUT subtable_chaining **classifiedST) {
	tableid_t compatibleCount = 0;
	classifier_hash *hb = NULL;
	classifier_hash *hi = NULL;
	classifier_hash *hf = NULL;
	// initialize the class hash
	subtable_chaining *subtable0 = &(lookup->subtables.items[j]->chaining);
	int classno_b = 0;
	int classno_i = 0;
	int classno_f = 0;

	otl_ChainingRule *rule0 = &subtable0->rule;
	for (tableid_t m = 0; m < rule0->matchCount; m++) {
		int check = 0;
		if (m < rule0->inputBegins) {
			check = classCompatible(&hb, rule0->match[m], &classno_b);
		} else if (m < rule0->inputEnds) {
			check = classCompatible(&hi, rule0->match[m], &classno_i);
		} else {
			check = classCompatible(&hf, rule0->match[m], &classno_f);
		}
		if (!check) { goto FAIL; }
	}
	for (tableid_t k = j + 1; k < lookup->subtables.length; k++) {
		otl_ChainingRule *rule = &lookup->subtables.items[k]->chaining.rule;
		bool allcheck = true;
		for (tableid_t m = 0; m < rule->matchCount; m++) {
			int check = 0;
			if (m < rule->inputBegins) {
				check = classCompatible(&hb, rule->match[m], &classno_b);
			} else if (m < rule->inputEnds) {
				check = classCompatible(&hi, rule->match[m], &classno_i);
			} else {
				check = classCompatible(&hf, rule->match[m], &classno_f);
			}
			if (!check) {
				allcheck = false;
				goto endcheck;
			}
		}
		if (allcheck) { compatibleCount += 1; }
	}
endcheck:
	if (compatibleCount > 1) {
		// We've found multiple compatible subtables;
		NEW(subtable0);
		subtable0->rulesCount = compatibleCount + 1;
		NEW(subtable0->rules, compatibleCount + 1);

		subtable0->rules[0] = buildRule(rule0, hb, hi, hf);
		// write other rules
		tableid_t kk = 1;
		for (tableid_t k = j + 1; k < lookup->subtables.length && kk < compatibleCount + 1; k++) {
			otl_ChainingRule *rule = &lookup->subtables.items[k]->chaining.rule;
			subtable0->rules[kk] = buildRule(rule, hb, hi, hf);
			kk++;
		}

		subtable0->type = otl_chaining_classified;
		subtable0->bc = toClass(&hb);
		subtable0->ic = toClass(&hi);
		subtable0->fc = toClass(&hf);
		*classifiedST = subtable0;
	}
FAIL:;
	if (hb) {
		classifier_hash *s, *tmp;
		HASH_ITER(hh, hb, s, tmp) {
			HASH_DEL(hb, s);
			FREE(s);
		}
	}
	if (hi) {
		classifier_hash *s, *tmp;
		HASH_ITER(hh, hi, s, tmp) {
			HASH_DEL(hi, s);
			FREE(s);
		}
	}
	if (hf) {
		classifier_hash *s, *tmp;
		HASH_ITER(hh, hf, s, tmp) {
			HASH_DEL(hf, s);
			FREE(s);
		}
	}

	if (compatibleCount > 1) {
		return compatibleCount;
	} else {
		return 0;
	}
}
tableid_t otfcc_classifiedBuildChaining(const otl_Lookup *lookup,
                                        OUT caryll_Buffer ***subtableBuffers,
                                        MODIFY size_t *lastOffset) {
	bool isContextual = otfcc_chainingLookupIsContextualLookup(lookup);
	tableid_t subtablesWritten = 0;
	NEW(*subtableBuffers, lookup->subtables.length);
	for (tableid_t j = 0; j < lookup->subtables.length; j++) {
		subtable_chaining *st0 = &(lookup->subtables.items[j]->chaining);
		if (st0->type) continue;
		subtable_chaining *st = st0;
		// Try to classify subtables after j into j
		j += tryClassifyAround(lookup, j, &st);
		caryll_Buffer *buf = isContextual ? otfcc_build_contextual((otl_Subtable *)st)
		                                  : otfcc_build_chaining((otl_Subtable *)st);
		if (st != st0) { iSubtable_chaining.free(st); }
		(*subtableBuffers)[subtablesWritten] = buf;
		*lastOffset += buf->size;
		subtablesWritten += 1;
	}
	return subtablesWritten;
}
