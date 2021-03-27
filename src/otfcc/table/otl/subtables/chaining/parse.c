#include "../chaining.h"

otl_Subtable *otl_parse_chaining(const json_value *_subtable, const otfcc_Options *options) {
	json_value *_match = json_obj_get_type(_subtable, "match", json_array);
	json_value *_apply = json_obj_get_type(_subtable, "apply", json_array);
	if (!_match || !_apply) return NULL;

	subtable_chaining *subtable = iSubtable_chaining.create();
	subtable->type = otl_chaining_canonical;

	otl_ChainingRule *rule = &subtable->rule;

	rule->matchCount = _match->u.array.length;
	NEW(rule->match, rule->matchCount);
	rule->applyCount = _apply->u.array.length;
	NEW(rule->apply, rule->applyCount);

	rule->inputBegins = json_obj_getnum_fallback(_subtable, "inputBegins", 0);
	rule->inputEnds = json_obj_getnum_fallback(_subtable, "inputEnds", rule->matchCount);

	for (tableid_t j = 0; j < rule->matchCount; j++) {
		rule->match[j] = Coverage.parse(_match->u.array.values[j]);
	}
	for (tableid_t j = 0; j < rule->applyCount; j++) {
		rule->apply[j].index = 0;
		rule->apply[j].lookup = Handle.empty();
		json_value *_application = _apply->u.array.values[j];
		if (_application->type == json_object) {
			json_value *_ln = json_obj_get_type(_application, "lookup", json_string);
			if (_ln) {
				rule->apply[j].lookup = Handle.fromName(sdsnewlen(_ln->u.string.ptr, _ln->u.string.length));
				rule->apply[j].index = json_obj_getnum(_application, "at");
			}
		}
	}
	return (otl_Subtable *)subtable;
}
