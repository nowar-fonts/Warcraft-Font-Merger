#include "chaining.h"

bool consolidate_chaining(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                          const otfcc_Options *options) {
	subtable_chaining *subtable = &(_subtable->chaining);
	if (subtable->type) {
		logWarning("[Consolidate] Ignoring non-canonical chaining subtable.");
		return false;
	}
	otl_ChainingRule *rule = &(subtable->rule);
	bool possible = true;
	for (tableid_t j = 0; j < rule->matchCount; j++) {
		fontop_consolidateCoverage(font, rule->match[j], options);
		Coverage.shrink(rule->match[j], true);
		possible = possible && (rule->match[j]->numGlyphs > 0);
	}
	if (rule->inputBegins > rule->matchCount) rule->inputBegins = rule->matchCount;
	if (rule->inputEnds > rule->matchCount) rule->inputEnds = rule->matchCount;
	for (tableid_t j = 0; j < rule->applyCount; j++) {
		bool foundLookup = false;
		lookup_handle *h = &(rule->apply[j].lookup);
		if (h->name) {
			for (tableid_t k = 0; k < table->lookups.length; k++) {
				if (!table->lookups.items[k]) continue;
				if (!table->lookups.items[k]->subtables.length) continue;
				if (strcmp(table->lookups.items[k]->name, h->name) != 0) continue;
				foundLookup = true;
				Handle.consolidateTo(h, k, table->lookups.items[k]->name);
			}
			if (!foundLookup && rule->apply[j].lookup.name) {
				logWarning("[Consolidate] Quoting an invalid lookup %s. This lookup application is "
				           "ignored.",
				           rule->apply[j].lookup.name);
				Handle.dispose(&rule->apply[j].lookup);
			}
		} else if (h->state == HANDLE_STATE_INDEX) {
			if (h->index >= table->lookups.length) {
				logWarning("[Consolidate] Quoting an invalid lookup #%d.", h->index);
				h->index = 0;
			}
			Handle.consolidateTo(h, h->index, table->lookups.items[h->index]->name);
		}
	}
	// If a rule is designed to have no lookup application, it may be a ignoration
	// otfcc will keep them.
	if (rule->applyCount) {
		tableid_t k = 0;
		for (tableid_t j = 0; j < rule->applyCount; j++) {
			if (rule->apply[j].lookup.name) { rule->apply[k++] = rule->apply[j]; }
		}
		rule->applyCount = k;
		if (!rule->applyCount) { return true; }
	}
	return !possible;
}
