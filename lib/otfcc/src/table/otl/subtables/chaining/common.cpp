#include "../chaining.h"
#include "common.h"
void otl_init_chaining(subtable_chaining *subtable) {
	memset(subtable, 0, sizeof(*subtable));
}
void otl_dispose_chaining(subtable_chaining *subtable) {
	if (subtable->type) {
		if (subtable->rules) {
			for (tableid_t j = 0; j < subtable->rulesCount; j++) {
				deleteRule(subtable->rules[j]);
			}
			FREE(subtable->rules);
		}
		if (subtable->bc) { ClassDef.free(subtable->bc); }
		if (subtable->ic) { ClassDef.free(subtable->ic); }
		if (subtable->fc) { ClassDef.free(subtable->fc); }
	} else {
		closeRule(&subtable->rule);
	}
}

caryll_standardRefType(subtable_chaining, iSubtable_chaining, otl_init_chaining,
                       otl_dispose_chaining);
