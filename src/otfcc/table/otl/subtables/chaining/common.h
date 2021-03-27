#include "../chaining.h"

static INLINE void closeRule(otl_ChainingRule *rule) {
	if (rule && rule->match && rule->matchCount) {
		for (tableid_t k = 0; k < rule->matchCount; k++) {
			Coverage.free(rule->match[k]);
		}
		FREE(rule->match);
	}
	if (rule && rule->apply) {
		for (tableid_t j = 0; j < rule->applyCount; j++) {
			Handle.dispose(&rule->apply[j].lookup);
		}
		FREE(rule->apply);
	}
}
static INLINE void deleteRule(otl_ChainingRule *rule) {
	if (!rule) return;
	closeRule(rule);
	FREE(rule);
}
static INLINE void reverseBacktracks(otl_ChainingRule *rule) {
	if (rule->inputBegins > 0) {
		tableid_t start = 0;
		tableid_t end = rule->inputBegins - 1;
		while (end > start) {
			otl_Coverage *tmp = rule->match[start];
			rule->match[start] = rule->match[end];
			rule->match[end] = tmp;
			end--, start++;
		}
	}
}
