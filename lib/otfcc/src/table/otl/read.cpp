#include "private.h"

#define LOOKUP_READER(llt, fn)                                                                     \
	case llt:                                                                                      \
		return fn(data, tableLength, subtableOffset, maxGlyphs, options);

otl_Subtable *otfcc_readOtl_subtable(font_file_pointer data, uint32_t tableLength,
                                     uint32_t subtableOffset, otl_LookupType lookupType,
                                     const glyphid_t maxGlyphs, const otfcc_Options *options) {
	switch (lookupType) {
		LOOKUP_READER(otl_type_gsub_single, otl_read_gsub_single);
		LOOKUP_READER(otl_type_gsub_multiple, otl_read_gsub_multi);
		LOOKUP_READER(otl_type_gsub_alternate, otl_read_gsub_multi);
		LOOKUP_READER(otl_type_gsub_ligature, otl_read_gsub_ligature);
		LOOKUP_READER(otl_type_gsub_chaining, otl_read_chaining);
		LOOKUP_READER(otl_type_gsub_reverse, otl_read_gsub_reverse);
		LOOKUP_READER(otl_type_gpos_chaining, otl_read_chaining);
		LOOKUP_READER(otl_type_gsub_context, otl_read_contextual);
		LOOKUP_READER(otl_type_gpos_context, otl_read_contextual);
		LOOKUP_READER(otl_type_gpos_single, otl_read_gpos_single);
		LOOKUP_READER(otl_type_gpos_pair, otl_read_gpos_pair);
		LOOKUP_READER(otl_type_gpos_cursive, otl_read_gpos_cursive);
		LOOKUP_READER(otl_type_gpos_markToBase, otl_read_gpos_markToSingle);
		LOOKUP_READER(otl_type_gpos_markToMark, otl_read_gpos_markToSingle);
		LOOKUP_READER(otl_type_gpos_markToLigature, otl_read_gpos_markToLigature);
		LOOKUP_READER(otl_type_gsub_extend, otfcc_readOtl_gsub_extend);
		LOOKUP_READER(otl_type_gpos_extend, otfcc_readOtl_gpos_extend);
		default:
			return NULL;
	}
}

static void parseLanguage(font_file_pointer data, uint32_t tableLength, uint32_t base,
                          otl_LanguageSystem *lang, otl_FeatureList *features) {
	checkLength(base + 6);
	tableid_t rid = read_16u(data + base + 2);
	if (rid < features->length) {
		lang->requiredFeature = features->items[rid];
	} else {
		lang->requiredFeature = NULL;
	}
	tableid_t featureCount = read_16u(data + base + 4);
	for (tableid_t j = 0; j < featureCount; j++) {
		tableid_t featureIndex = read_16u(data + base + 6 + 2 * j);
		if (featureIndex < features->length) {
			otl_iFeatureRefList.push(&lang->features, features->items[featureIndex]);
		}
	}
	return;
FAIL:
	otl_iFeatureRefList.dispose(&lang->features);
	lang->requiredFeature = NULL;
	return;
}

static table_OTL *otfcc_readOtl_common(font_file_pointer data, uint32_t tableLength,
                                       otl_LookupType lookup_type_base,
                                       const otfcc_Options *options) {
	table_OTL *table = table_iOTL.create();
	if (!table) goto FAIL;
	checkLength(10);
	uint32_t scriptListOffset = read_16u(data + 4);
	checkLength(scriptListOffset + 2);
	uint32_t featureListOffset = read_16u(data + 6);
	checkLength(featureListOffset + 2);
	uint32_t lookupListOffset = read_16u(data + 8);
	checkLength(lookupListOffset + 2);

	// parse lookup list
	{
		tableid_t lookupCount = read_16u(data + lookupListOffset);
		checkLength(lookupListOffset + 2 + lookupCount * 2);
		for (tableid_t j = 0; j < lookupCount; j++) {
			otl_Lookup *lookup;
			otl_iLookupPtr.init(&lookup);
			lookup->_offset = lookupListOffset + read_16u(data + lookupListOffset + 2 + 2 * j);
			checkLength(lookup->_offset + 6);
			lookup->type = read_16u(data + lookup->_offset) + lookup_type_base;
			otl_iLookupList.push(&table->lookups, lookup);
		}
	}

	// parse feature list
	{
		tableid_t featureCount = read_16u(data + featureListOffset);
		checkLength(featureListOffset + 2 + featureCount * 6);
		tableid_t lnk = 0;
		for (tableid_t j = 0; j < featureCount; j++) {
			otl_Feature *feature;
			otl_iFeaturePtr.init(&feature);
			uint32_t tag = read_32u(data + featureListOffset + 2 + j * 6);
			if (options->glyph_name_prefix) {
				feature->name = sdscatprintf(sdsempty(), "%c%c%c%c_%s_%05d", (tag >> 24) & 0xFF,
				                             (tag >> 16) & 0xFF, (tag >> 8) & 0xff, tag & 0xff,
				                             options->glyph_name_prefix, j);
			} else {
				feature->name = sdscatprintf(sdsempty(), "%c%c%c%c_%05d", (tag >> 24) & 0xFF,
				                             (tag >> 16) & 0xFF, (tag >> 8) & 0xff, tag & 0xff, j);
			}
			uint32_t featureOffset =
			    featureListOffset + read_16u(data + featureListOffset + 2 + j * 6 + 4);

			checkLength(featureOffset + 4);
			tableid_t lookupCount = read_16u(data + featureOffset + 2);
			checkLength(featureOffset + 4 + lookupCount * 2);
			for (tableid_t k = 0; k < lookupCount; k++) {
				tableid_t lookupid = read_16u(data + featureOffset + 4 + k * 2);
				if (lookupid < table->lookups.length) {
					otl_Lookup *lookup = table->lookups.items[lookupid];
					if (!lookup->name) {
						if (options->glyph_name_prefix) {
							lookup->name = sdscatprintf(sdsempty(), "lookup_%s_%c%c%c%c_%d",
							                            options->glyph_name_prefix,
							                            (tag >> 24) & 0xFF, (tag >> 16) & 0xFF,
							                            (tag >> 8) & 0xff, tag & 0xff, lnk++);
						} else {
							lookup->name = sdscatprintf(sdsempty(), "lookup_%c%c%c%c_%d",
							                            (tag >> 24) & 0xFF, (tag >> 16) & 0xFF,
							                            (tag >> 8) & 0xff, tag & 0xff, lnk++);
						}
					}
					otl_iLookupRefList.push(&feature->lookups, lookup);
				}
			}
			otl_iFeatureList.push(&table->features, feature);
		}
	}

	// parse script list
	{
		tableid_t scriptCount = read_16u(data + scriptListOffset);
		checkLength(scriptListOffset + 2 + 6 * scriptCount);

		uint32_t nLanguageCombinations = 0;
		for (tableid_t j = 0; j < scriptCount; j++) {
			uint32_t scriptOffset =
			    scriptListOffset + read_16u(data + scriptListOffset + 2 + 6 * j + 4);
			checkLength(scriptOffset + 4);

			tableid_t defaultLangSystem = read_16u(data + scriptOffset);
			nLanguageCombinations +=
			    (defaultLangSystem ? 1 : 0) + read_16u(data + scriptOffset + 2);
		}

		for (tableid_t j = 0; j < scriptCount; j++) {
			uint32_t tag = read_32u(data + scriptListOffset + 2 + 6 * j);
			uint32_t scriptOffset =
			    scriptListOffset + read_16u(data + scriptListOffset + 2 + 6 * j + 4);
			tableid_t defaultLangSystem = read_16u(data + scriptOffset);
			if (defaultLangSystem) {
				otl_LanguageSystem *lang;
				otl_iLanguageSystem.init(&lang);
				lang->name = sdscatprintf(sdsempty(), "%c%c%c%c%cDFLT", (tag >> 24) & 0xFF,
				                          (tag >> 16) & 0xFF, (tag >> 8) & 0xff, tag & 0xff,
				                          SCRIPT_LANGUAGE_SEPARATOR);
				parseLanguage(data, tableLength, scriptOffset + defaultLangSystem, lang,
				              &table->features);
				otl_iLangSystemList.push(&table->languages, lang);
			}
			tableid_t langSysCount = read_16u(data + scriptOffset + 2);
			for (tableid_t k = 0; k < langSysCount; k++) {
				uint32_t langTag = read_32u(data + scriptOffset + 4 + 6 * k);
				tableid_t langSys = read_16u(data + scriptOffset + 4 + 6 * k + 4);
				otl_LanguageSystem *lang;
				otl_iLanguageSystem.init(&lang);
				lang->name =
				    sdscatprintf(sdsempty(), "%c%c%c%c%c%c%c%c%c", (tag >> 24) & 0xFF,
				                 (tag >> 16) & 0xFF, (tag >> 8) & 0xff, tag & 0xff,
				                 SCRIPT_LANGUAGE_SEPARATOR, (langTag >> 24) & 0xFF,
				                 (langTag >> 16) & 0xFF, (langTag >> 8) & 0xff, langTag & 0xff);
				parseLanguage(data, tableLength, scriptOffset + langSys, lang, &table->features);
				otl_iLangSystemList.push(&table->languages, lang);
			}
		}
	}

	// name all lookups
	for (tableid_t j = 0; j < table->lookups.length; j++) {
		if (!table->lookups.items[j]->name) {
			if (options->glyph_name_prefix) {
				table->lookups.items[j]->name =
				    sdscatprintf(sdsempty(), "lookup_%s_%02x_%d", options->glyph_name_prefix,
				                 table->lookups.items[j]->type, j);
			} else {
				table->lookups.items[j]->name =
				    sdscatprintf(sdsempty(), "lookup_%02x_%d", table->lookups.items[j]->type, j);
			}
		}
	}
	return table;
FAIL:
	if (table) table_iOTL.free(table);
	return NULL;
}

static void otfcc_readOtl_lookup(font_file_pointer data, uint32_t tableLength, otl_Lookup *lookup,
                                 glyphid_t maxGlyphs, const otfcc_Options *options) {
	lookup->flags = read_16u(data + lookup->_offset + 2);
	tableid_t subtableCount = read_16u(data + lookup->_offset + 4);
	if (!subtableCount || tableLength < lookup->_offset + 6 + 2 * subtableCount) {
		lookup->type = otl_type_unknown;
		return;
	}
	for (tableid_t j = 0; j < subtableCount; j++) {
		uint32_t subtableOffset = lookup->_offset + read_16u(data + lookup->_offset + 6 + j * 2);
		otl_Subtable *subtable = otfcc_readOtl_subtable(data, tableLength, subtableOffset,
		                                                lookup->type, maxGlyphs, options);
		otl_iSubtableList.push(&lookup->subtables, subtable);
	}
	if (lookup->type == otl_type_gsub_extend || lookup->type == otl_type_gpos_extend) {
		lookup->type = 0;
		for (tableid_t j = 0; j < lookup->subtables.length; j++) {
			if (lookup->subtables.items[j]) {
				lookup->type = lookup->subtables.items[j]->extend.type;
				break;
			}
		}
		if (lookup->type) {
			for (tableid_t j = 0; j < lookup->subtables.length; j++) {
				if (lookup->subtables.items[j] &&
				    lookup->subtables.items[j]->extend.type == lookup->type) {
					// this subtable is valid
					otl_Subtable *st = lookup->subtables.items[j]->extend.subtable;
					FREE(lookup->subtables.items[j]);
					lookup->subtables.items[j] = st;
				} else if (lookup->subtables.items[j]) {
					// type mismatch, delete this subtable
					otl_Lookup *temp;
					otl_iLookupPtr.init(&temp);
					temp->type = lookup->subtables.items[j]->extend.type;
					otl_iSubtableList.push(&temp->subtables,
					                       lookup->subtables.items[j]->extend.subtable);
					DELETE(otfcc_delete_lookup, temp);
					FREE(lookup->subtables.items[j]);
				}
			}
		} else {
			otl_iSubtableList.disposeDependent(&lookup->subtables, lookup);
			return;
		}
	}
	if (lookup->type == otl_type_gsub_context) lookup->type = otl_type_gsub_chaining;
	if (lookup->type == otl_type_gpos_context) lookup->type = otl_type_gpos_chaining;
}

table_OTL *otfcc_readOtl(otfcc_Packet packet, const otfcc_Options *options, uint32_t tag,
                         glyphid_t maxGlyphs) {
	table_OTL *otl = NULL;
	FOR_TABLE(tag, table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		otl = otfcc_readOtl_common(data, length,
		                           (tag == 'GSUB'
		                                ? otl_type_gsub_unknown
		                                : tag == 'GPOS' ? otl_type_gpos_unknown : otl_type_unknown),
		                           options);
		if (!otl) goto FAIL;
		for (tableid_t j = 0; j < otl->lookups.length; j++) {
			otfcc_readOtl_lookup(data, length, otl->lookups.items[j], maxGlyphs, options);
		}
		return otl;
	FAIL:
		if (otl) table_iOTL.free(otl);
		otl = NULL;
	}
	return NULL;
}
