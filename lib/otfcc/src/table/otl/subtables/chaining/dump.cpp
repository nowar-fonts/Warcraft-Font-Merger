#include "../chaining.h"

json_value *otl_dump_chaining(const otl_Subtable *_subtable) {
	const subtable_chaining *subtable = &(_subtable->chaining);
	if (subtable->type) return json_null_new();
	const otl_ChainingRule *rule = &(subtable->rule);
	json_value *_st = json_object_new(4);

	json_value *_match = json_array_new(rule->matchCount);
	for (tableid_t j = 0; j < rule->matchCount; j++) {
		json_array_push(_match, Coverage.dump(rule->match[j]));
	}
	json_object_push(_st, "match", _match);

	json_value *_apply = json_array_new(rule->applyCount);
	for (tableid_t j = 0; j < rule->applyCount; j++) {
		json_value *_application = json_object_new(2);
		json_object_push(_application, "at", json_integer_new(rule->apply[j].index));
		json_object_push(_application, "lookup", json_string_new(rule->apply[j].lookup.name));
		json_array_push(_apply, _application);
	}
	json_object_push(_st, "apply", preserialize(_apply));

	json_object_push(_st, "inputBegins", json_integer_new(rule->inputBegins));
	json_object_push(_st, "inputEnds", json_integer_new(rule->inputEnds));
	return _st;
}
