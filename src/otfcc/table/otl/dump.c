#include "private.h"

static void _declare_lookup_dumper(otl_LookupType llt, const char *lt, json_value *(*dumper)(const otl_Subtable *st),
                                   otl_Lookup *lookup, json_value *dump) {
	if (lookup->type == llt) {
		json_object_push(dump, "type", json_string_new(lt));
		json_object_push(dump, "flags", otfcc_dump_flags(lookup->flags, lookupFlagsLabels));
		if (lookup->flags >> 8) { json_object_push(dump, "markAttachmentType", json_integer_new(lookup->flags >> 8)); }
		json_value *subtables = json_array_new(lookup->subtables.length);
		for (tableid_t j = 0; j < lookup->subtables.length; j++)
			if (lookup->subtables.items[j]) { json_array_push(subtables, dumper(lookup->subtables.items[j])); }
		json_object_push(dump, "subtables", subtables);
	}
}

#define LOOKUP_DUMPER(llt, fn) _declare_lookup_dumper(llt, tableNames[llt], fn, lookup, dump);

static void _dump_lookup(otl_Lookup *lookup, json_value *dump) {
	LOOKUP_DUMPER(otl_type_gsub_single, otl_gsub_dump_single);
	LOOKUP_DUMPER(otl_type_gsub_multiple, otl_gsub_dump_multi);
	LOOKUP_DUMPER(otl_type_gsub_alternate, otl_gsub_dump_multi);
	LOOKUP_DUMPER(otl_type_gsub_ligature, otl_gsub_dump_ligature);
	LOOKUP_DUMPER(otl_type_gsub_chaining, otl_dump_chaining);
	LOOKUP_DUMPER(otl_type_gsub_reverse, otl_gsub_dump_reverse);
	LOOKUP_DUMPER(otl_type_gpos_chaining, otl_dump_chaining);
	LOOKUP_DUMPER(otl_type_gpos_single, otl_gpos_dump_single);
	LOOKUP_DUMPER(otl_type_gpos_pair, otl_gpos_dump_pair);
	LOOKUP_DUMPER(otl_type_gpos_cursive, otl_gpos_dump_cursive);
	LOOKUP_DUMPER(otl_type_gpos_markToBase, otl_gpos_dump_markToSingle);
	LOOKUP_DUMPER(otl_type_gpos_markToMark, otl_gpos_dump_markToSingle);
	LOOKUP_DUMPER(otl_type_gpos_markToLigature, otl_gpos_dump_markToLigature);
}

void otfcc_dumpOtl(const table_OTL *table, json_value *root, const otfcc_Options *options, const char *tag) {
	if (!table || !table->languages.length || !table->lookups.length || !table->features.length) return;
	loggedStep("%s", tag) {
		json_value *otl = json_object_new(3);
		loggedStep("Languages") {
			// dump script list
			json_value *languages = json_object_new(table->languages.length);
			for (tableid_t j = 0; j < table->languages.length; j++) {
				json_value *_lang = json_object_new(5);
				otl_LanguageSystem *lang = table->languages.items[j];
				if (lang->requiredFeature) {
					json_object_push(_lang, "requiredFeature", json_string_new(lang->requiredFeature->name));
				}
				json_value *features = json_array_new(lang->features.length);
				for (tableid_t k = 0; k < lang->features.length; k++)
					if (lang->features.items[k]) {
						json_array_push(features, json_string_new(lang->features.items[k]->name));
					}
				json_object_push(_lang, "features", preserialize(features));
				json_object_push(languages, lang->name, _lang);
			}
			json_object_push(otl, "languages", languages);
		}
		loggedStep("Features") {
			// dump feature list
			json_value *features = json_object_new(table->features.length);
			for (tableid_t j = 0; j < table->features.length; j++) {
				otl_Feature *feature = table->features.items[j];
				json_value *_feature = json_array_new(feature->lookups.length);
				for (tableid_t k = 0; k < feature->lookups.length; k++)
					if (feature->lookups.items[k]) {
						json_array_push(_feature, json_string_new(feature->lookups.items[k]->name));
					}
				json_object_push(features, feature->name, preserialize(_feature));
			}
			json_object_push(otl, "features", features);
		}
		loggedStep("Lookups") {
			// dump lookups
			json_value *lookups = json_object_new(table->lookups.length);
			json_value *lookupOrder = json_array_new(table->lookups.length);
			for (tableid_t j = 0; j < table->lookups.length; j++) {
				json_value *_lookup = json_object_new(5);
				otl_Lookup *lookup = table->lookups.items[j];
				_dump_lookup(lookup, _lookup);
				json_object_push(lookups, lookup->name, _lookup);
				json_array_push(lookupOrder, json_string_new(lookup->name));
			}
			json_object_push(otl, "lookups", lookups);
			json_object_push(otl, "lookupOrder", lookupOrder);
		}
		json_object_push(root, tag, otl);
	}
}
