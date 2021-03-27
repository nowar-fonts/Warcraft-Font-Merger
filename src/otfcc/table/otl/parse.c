#include "private.h"

typedef enum { LOOKUP_ORDER_FORCE, LOOKUP_ORDER_FILE } lookup_order_type;

typedef struct {
	char *name;
	otl_Lookup *lookup;
	UT_hash_handle hh;
	lookup_order_type orderType;
	uint16_t orderVal;
} lookup_hash;

typedef struct {
	char *name;
	bool alias;
	otl_Feature *feature;
	UT_hash_handle hh;
} feature_hash;

typedef struct {
	char *name;
	otl_LanguageSystem *language;
	UT_hash_handle hh;
} language_hash;
static bool _declareLookupParser(const char *lt, otl_LookupType llt,
                                 otl_Subtable *(*parser)(const json_value *,
                                                         const otfcc_Options *options),
                                 json_value *_lookup, char *lookupName,
                                 const otfcc_Options *options, lookup_hash **lh);

#define LOOKUP_PARSER(llt, parser)                                                                 \
	if (!parsed) {                                                                                 \
		parsed =                                                                                   \
		    _declareLookupParser(tableNames[llt], llt, parser, lookup, lookupName, options, lh);   \
	}

static bool _parse_lookup(json_value *lookup, char *lookupName, const otfcc_Options *options,
                          lookup_hash **lh) {
	bool parsed = false;
	LOOKUP_PARSER(otl_type_gsub_single, otl_gsub_parse_single);
	LOOKUP_PARSER(otl_type_gsub_multiple, otl_gsub_parse_multi);
	LOOKUP_PARSER(otl_type_gsub_alternate, otl_gsub_parse_multi);
	LOOKUP_PARSER(otl_type_gsub_ligature, otl_gsub_parse_ligature);
	LOOKUP_PARSER(otl_type_gsub_chaining, otl_parse_chaining);
	LOOKUP_PARSER(otl_type_gsub_reverse, otl_gsub_parse_reverse);
	LOOKUP_PARSER(otl_type_gpos_single, otl_gpos_parse_single);
	LOOKUP_PARSER(otl_type_gpos_pair, otl_gpos_parse_pair);
	LOOKUP_PARSER(otl_type_gpos_cursive, otl_gpos_parse_cursive);
	LOOKUP_PARSER(otl_type_gpos_chaining, otl_parse_chaining);
	LOOKUP_PARSER(otl_type_gpos_markToBase, otl_gpos_parse_markToSingle);
	LOOKUP_PARSER(otl_type_gpos_markToMark, otl_gpos_parse_markToSingle);
	LOOKUP_PARSER(otl_type_gpos_markToLigature, otl_gpos_parse_markToLigature);
	return parsed;
}

static bool _declareLookupParser(const char *lt, otl_LookupType llt,
                                 otl_Subtable *(*parser)(const json_value *,
                                                         const otfcc_Options *options),
                                 json_value *_lookup, char *lookupName,
                                 const otfcc_Options *options, lookup_hash **lh) {

	// detect a valid type field exists
	json_value *type = json_obj_get_type(_lookup, "type", json_string);
	if (!type || strcmp(type->u.string.ptr, lt)) {
		if (!type) logWarning("Lookup %s does not have a valid 'type' field.", lookupName);
		return false;
	}
	// no duplicate lookup has been parsed
	lookup_hash *item = NULL;
	HASH_FIND_STR(*lh, lookupName, item);
	if (item) {
		logWarning("Lookup %s already exists.", lookupName);
		return false;
	}
	// detect valid subtables array
	json_value *_subtables = json_obj_get_type(_lookup, "subtables", json_array);
	if (!_subtables) {
		logWarning("Lookup %s does not have a valid subtable list.", lookupName);
		return false;
	}
	// init this lookup
	otl_Lookup *lookup;
	otl_iLookupPtr.init(&lookup);
	lookup->type = llt;
	lookup->flags = otfcc_parse_flags(json_obj_get(_lookup, "flags"), lookupFlagsLabels);
	uint16_t markAttachmentType = json_obj_getint(_lookup, "markAttachmentType");
	if (markAttachmentType) { lookup->flags |= markAttachmentType << 8; }
	// start parse subtables
	tableid_t subtableCount = _subtables->u.array.length;
	loggedStep("%s", lookupName) {
		for (tableid_t j = 0; j < subtableCount; j++) {
			json_value *_subtable = _subtables->u.array.values[j];
			if (_subtable && _subtable->type == json_object) {
				otl_Subtable *_st = parser(_subtable, options);
				otl_iSubtableList.push(&lookup->subtables, _st);
			}
		}
	}
	if (!lookup->subtables.length) {
		logWarning("Lookup %s does not have any subtables.", lookupName);
		otfcc_delete_lookup(lookup);
		return false;
	}
	// we've found a valid lookup, put it into the hash.
	NEW(item);
	item->name = sdsnew(lookupName);
	lookup->name = sdsdup(item->name);
	item->lookup = lookup;
	item->orderType = LOOKUP_ORDER_FILE;
	item->orderVal = HASH_COUNT(*lh);
	HASH_ADD_STR(*lh, name, item);
	return true;
}

static lookup_hash *figureOutLookupsFromJSON(json_value *lookups, const otfcc_Options *options) {
	lookup_hash *lh = NULL;

	for (uint32_t j = 0; j < lookups->u.object.length; j++) {
		char *lookupName = lookups->u.object.values[j].name;
		if (lookups->u.object.values[j].value->type == json_object) {
			bool parsed =
			    _parse_lookup(lookups->u.object.values[j].value, lookupName, options, &lh);
			if (!parsed) {
				logWarning("[OTFCC-fea] Ignoring invalid or unsupported lookup %s.\n", lookupName);
			}
		} else if (lookups->u.object.values[j].value->type == json_string) {
			char *thatname = lookups->u.object.values[j].value->u.string.ptr;
			lookup_hash *s = NULL;
			HASH_FIND_STR(lh, thatname, s);
			if (s) {
				lookup_hash *dup;
				NEW(dup);
				dup->name = sdsnew(lookupName);
				dup->lookup = s->lookup;
				dup->orderType = LOOKUP_ORDER_FILE;
				dup->orderVal = HASH_COUNT(lh);
				HASH_ADD_STR(lh, name, dup);
			}
		}
	}
	return lh;
}

static void feature_merger_activate(json_value *d, const bool sametag, const char *objtype,
                                    const otfcc_Options *options) {
	for (uint32_t j = 0; j < d->u.object.length; j++) {
		json_value *jthis = d->u.object.values[j].value;
		char *kthis = d->u.object.values[j].name;
		uint32_t nkthis = d->u.object.values[j].name_length;
		if (jthis->type != json_array && jthis->type != json_object) continue;
		for (uint32_t k = j + 1; k < d->u.object.length; k++) {
			json_value *jthat = d->u.object.values[k].value;
			char *kthat = d->u.object.values[k].name;
			if (json_ident(jthis, jthat) && (sametag ? strncmp(kthis, kthat, 4) == 0 : true)) {
				json_value_free(jthat);
				json_value *v = json_string_new_length(nkthis, kthis);
				v->parent = d;
				d->u.object.values[k].value = v;
				logNotice("[OTFCC-fea] Merged duplicate %s '%s' into '%s'.\n", objtype, kthat,
				          kthis);
			}
		}
	}
}

static feature_hash *figureOutFeaturesFromJSON(json_value *features, lookup_hash *lh,
                                               const char *tag, const otfcc_Options *options) {
	feature_hash *fh = NULL;
	// Remove duplicates
	if (options->merge_features) { feature_merger_activate(features, true, "feature", options); }
	// Resolve features
	for (uint32_t j = 0; j < features->u.object.length; j++) {
		char *featureName = features->u.object.values[j].name;
		json_value *_feature = features->u.object.values[j].value;
		if (_feature->type == json_array) {
			otl_LookupRefList al;
			otl_iLookupRefList.init(&al);
			for (tableid_t k = 0; k < _feature->u.array.length; k++) {
				json_value *term = _feature->u.array.values[k];
				if (term->type != json_string) continue;
				lookup_hash *item = NULL;
				HASH_FIND_STR(lh, term->u.string.ptr, item);
				if (item) {
					otl_iLookupRefList.push(&al, item->lookup);
				} else {
					logWarning("Lookup assignment %s for feature [%s/%s] is missing or invalid.",
					           term->u.string.ptr, tag, featureName)
				}
			}
			if (al.length > 0) {
				feature_hash *s = NULL;
				HASH_FIND_STR(fh, featureName, s);
				if (!s) {
					NEW(s);
					s->name = sdsnew(featureName);
					s->alias = false;
					otl_iFeaturePtr.init(&s->feature);
					s->feature->name = sdsdup(s->name);
					otl_iLookupRefList.replace(&s->feature->lookups, al);
					HASH_ADD_STR(fh, name, s);
				} else {
					logWarning("[OTFCC-fea] Duplicate feature for [%s/%s]. This feature will "
					           "be ignored.\n",
					           tag, featureName);
					otl_iLookupRefList.dispose(&al);
				}
			} else {
				logWarning("[OTFCC-fea] There is no valid lookup "
				           "assignments for [%s/%s]. This feature will be "
				           "ignored.\n",
				           tag, featureName);
				otl_iLookupRefList.dispose(&al);
			}
		} else if (_feature->type == json_string) {
			feature_hash *s = NULL;
			char *target = _feature->u.string.ptr;
			HASH_FIND_STR(fh, target, s);
			if (s) {
				feature_hash *dup;
				NEW(dup);
				dup->alias = true;
				dup->name = sdsnew(featureName);
				dup->feature = s->feature;
				HASH_ADD_STR(fh, name, dup);
			}
		}
	}
	return fh;
}
bool isValidLanguageName(const char *name, const size_t length) {
	return length == 9 && name[4] == SCRIPT_LANGUAGE_SEPARATOR;
}
static language_hash *figureOutLanguagesFromJson(json_value *languages, feature_hash *fh,
                                                 const char *tag, const otfcc_Options *options) {
	language_hash *sh = NULL;
	// languages
	for (uint32_t j = 0; j < languages->u.object.length; j++) {
		char *languageName = languages->u.object.values[j].name;
		size_t languageNameLen = languages->u.object.values[j].name_length;
		json_value *_language = languages->u.object.values[j].value;
		if (isValidLanguageName(languageName, languageNameLen) && _language->type == json_object) {
			otl_Feature *requiredFeature = NULL;
			json_value *_rf = json_obj_get_type(_language, "requiredFeature", json_string);
			if (_rf) {
				// required feature term
				feature_hash *rf = NULL;
				HASH_FIND_STR(fh, _rf->u.string.ptr, rf);
				if (rf) { requiredFeature = rf->feature; }
			}
			otl_FeatureRefList af;
			otl_iFeatureRefList.init(&af);
			json_value *_features = json_obj_get_type(_language, "features", json_array);
			if (_features) {
				for (tableid_t k = 0; k < _features->u.array.length; k++) {
					json_value *term = _features->u.array.values[k];
					if (term->type == json_string) {
						feature_hash *item = NULL;
						HASH_FIND_STR(fh, term->u.string.ptr, item);
						if (item) { otl_iFeatureRefList.push(&af, item->feature); }
					}
				}
			}
			if (requiredFeature || (af.length > 0)) {
				language_hash *s = NULL;
				HASH_FIND_STR(sh, languageName, s);
				if (!s) {
					NEW(s);
					s->name = sdsnew(languageName);
					otl_iLanguageSystem.init(&s->language);
					s->language->name = sdsdup(s->name);
					s->language->requiredFeature = requiredFeature;
					otl_iFeatureRefList.replace(&s->language->features, af);
					HASH_ADD_STR(sh, name, s);
				} else {
					logWarning("[OTFCC-fea] Duplicate language item [%s/%s]. This language "
					           "term will be ignored.\n",
					           tag, languageName);
					otl_iFeatureRefList.dispose(&af);
				}
			} else {
				logWarning("[OTFCC-fea] There is no valid feature "
				           "assignments for [%s/%s]. This language term "
				           "will be ignored.\n",
				           tag, languageName);
				otl_iFeatureRefList.dispose(&af);
			}
		}
	}
	return sh;
}

static int by_lookup_order(lookup_hash *a, lookup_hash *b) {
	if (a->orderType == b->orderType) {
		return a->orderVal - b->orderVal;
	} else {
		return a->orderType - b->orderType;
	}
}
static int by_feature_name(feature_hash *a, feature_hash *b) {
	return strcmp(a->name, b->name);
}
static int by_language_name(language_hash *a, language_hash *b) {
	return strcmp(a->name, b->name);
}
table_OTL *otfcc_parseOtl(const json_value *root, const otfcc_Options *options, const char *tag) {
	table_OTL *otl = NULL;
	json_value *table = json_obj_get_type(root, tag, json_object);
	if (!table) goto FAIL;
	otl = table_iOTL.create();
	json_value *languages = json_obj_get_type(table, "languages", json_object);
	json_value *features = json_obj_get_type(table, "features", json_object);
	json_value *lookups = json_obj_get_type(table, "lookups", json_object);
	if (!languages || !features || !lookups) goto FAIL;

	loggedStep("%s", tag) {
		lookup_hash *lh = figureOutLookupsFromJSON(lookups, options);
		json_value *lookupOrder = json_obj_get_type(table, "lookupOrder", json_array);
		if (lookupOrder) {
			for (tableid_t j = 0; j < lookupOrder->u.array.length; j++) {
				json_value *_ln = lookupOrder->u.array.values[j];
				if (_ln && _ln->type == json_string) {
					lookup_hash *item = NULL;
					HASH_FIND_STR(lh, _ln->u.string.ptr, item);
					if (item) {
						item->orderType = LOOKUP_ORDER_FORCE;
						item->orderVal = j;
					}
				}
			}
		}
		HASH_SORT(lh, by_lookup_order);
		feature_hash *fh = figureOutFeaturesFromJSON(features, lh, tag, options);
		HASH_SORT(fh, by_feature_name);
		language_hash *sh = figureOutLanguagesFromJson(languages, fh, tag, options);
		HASH_SORT(sh, by_language_name);
		if (!HASH_COUNT(lh) || !HASH_COUNT(fh) || !HASH_COUNT(sh)) {
			options->logger->dedent(options->logger);
			goto FAIL;
		}

		{
			lookup_hash *s, *tmp;
			HASH_ITER(hh, lh, s, tmp) {
				otl_iLookupList.push(&otl->lookups, s->lookup);
				HASH_DEL(lh, s);
				sdsfree(s->name);
				FREE(s);
			}
		}
		{
			feature_hash *s, *tmp;
			HASH_ITER(hh, fh, s, tmp) {
				if (!s->alias) { otl_iFeatureList.push(&otl->features, s->feature); }
				HASH_DEL(fh, s);
				sdsfree(s->name);
				FREE(s);
			}
		}
		{
			language_hash *s, *tmp;
			HASH_ITER(hh, sh, s, tmp) {
				otl_iLangSystemList.push(&otl->languages, s->language);
				HASH_DEL(sh, s);
				sdsfree(s->name);
				FREE(s);
			}
		}
	}
	return otl;
FAIL:
	if (otl) {
		logWarning("[OTFCC-fea] Ignoring invalid or incomplete OTL table %s.\n", tag);
		table_iOTL.free(otl);
	}
	return NULL;
}
