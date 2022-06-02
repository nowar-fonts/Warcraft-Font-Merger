#include "private.h"

#define LARGE_SUBTABLE_LIMIT 4096

static uint32_t featureNameToTag(const sds name) {
	uint32_t tag = 0;
	if (sdslen(name) > 0) {
		tag |= ((uint8_t)name[0]) << 24;
	} else {
		tag |= ((uint8_t)' ') << 24;
	}
	if (sdslen(name) > 1) {
		tag |= ((uint8_t)name[1]) << 16;
	} else {
		tag |= ((uint8_t)' ') << 16;
	}
	if (sdslen(name) > 2) {
		tag |= ((uint8_t)name[2]) << 8;
	} else {
		tag |= ((uint8_t)' ') << 8;
	}
	if (sdslen(name) > 3) {
		tag |= ((uint8_t)name[3]) << 0;
	} else {
		tag |= ((uint8_t)' ') << 0;
	}
	return tag;
}

typedef caryll_Buffer *(*_otl_Builder)(const otl_Subtable *_subtable,
                                       otl_BuildHeuristics heuristics);

static tableid_t _declare_lookup_writer(otl_LookupType type, _otl_Builder fn,
                                        const otl_Lookup *lookup, caryll_Buffer ***subtables,
                                        size_t *lastOffset, bool *preferExtensionForThisLUT,
                                        otl_BuildHeuristics heuristics) {
	if (lookup->type == type) {
		NEW(*subtables, lookup->subtables.length);
		size_t totalBufSizeShort = 0;
		size_t totalBufSizeExt = 0;
		for (tableid_t j = 0; j < lookup->subtables.length; j++) {
			caryll_Buffer *buf = fn(lookup->subtables.items[j], heuristics);
			(*subtables)[j] = buf;
			totalBufSizeShort += buf->size;
			totalBufSizeExt += 8;
		}
		if (totalBufSizeShort > LARGE_SUBTABLE_LIMIT) {
			*lastOffset += totalBufSizeExt;
			*preferExtensionForThisLUT = true;
		} else {
			*lastOffset += totalBufSizeShort;
			*preferExtensionForThisLUT = false;
		}
		return lookup->subtables.length;
	}
	return 0;
}

#define LOOKUP_WRITER(type, fn)                                                                    \
	if (!written)                                                                                  \
		written = _declare_lookup_writer(type, fn, lookup, subtables, lastOffset,                  \
		                                 preferExtensionForThisLUT, heuristics);

static tableid_t _build_lookup(const otl_Lookup *lookup, caryll_Buffer ***subtables,
                               size_t *lastOffset, bool *preferExtensionForThisLUT,
                               otl_BuildHeuristics heuristics) {
	if (lookup->type == otl_type_gpos_chaining || lookup->type == otl_type_gsub_chaining) {
		return otfcc_classifiedBuildChaining(lookup, subtables, lastOffset);
	}

	tableid_t written = 0;
	LOOKUP_WRITER(otl_type_gsub_single, otfcc_build_gsub_single_subtable);
	LOOKUP_WRITER(otl_type_gsub_multiple, otfcc_build_gsub_multi_subtable);
	LOOKUP_WRITER(otl_type_gsub_alternate, otfcc_build_gsub_multi_subtable);
	LOOKUP_WRITER(otl_type_gsub_ligature, otfcc_build_gsub_ligature_subtable);
	LOOKUP_WRITER(otl_type_gsub_reverse, otfcc_build_gsub_reverse);
	LOOKUP_WRITER(otl_type_gpos_single, otfcc_build_gpos_single);
	LOOKUP_WRITER(otl_type_gpos_pair, otfcc_build_gpos_pair);
	LOOKUP_WRITER(otl_type_gpos_cursive, otfcc_build_gpos_cursive);
	LOOKUP_WRITER(otl_type_gpos_markToBase, otfcc_build_gpos_markToSingle);
	LOOKUP_WRITER(otl_type_gpos_markToMark, otfcc_build_gpos_markToSingle);
	LOOKUP_WRITER(otl_type_gpos_markToLigature, otfcc_build_gpos_markToLigature);
	return written;
}

static otl_BuildHeuristics getLookupHeuristics(const table_OTL *table, const otl_Lookup *lut) {
	otl_BuildHeuristics heu = OTL_BH_NORMAL;
	// GSUB VERT heuristics
	// GDI have some restrictions on the internal format of the lookup inisde a VERT feature
	if (lut->type == otl_type_gsub_single) {
		for (tableid_t j = 0; j < table->features.length; j++) {
			const otl_Feature *fea = table->features.items[j];
			if (featureNameToTag(fea->name) != 'vert') continue;
			for (tableid_t k = 0; k < fea->lookups.length; k++) {
				if (fea->lookups.items[k] == lut) heu |= OTL_BH_GSUB_VERT;
			}
		}
	}
	return heu;
}

// When writing lookups, otfcc will try to maintain everything correctly.
// That is, we will use extended layout lookups automatically when the
// offsets are too large.
static bk_Block *writeOTLLookups(const table_OTL *table, const otfcc_Options *options,
                                 const char *tag) {
	caryll_Buffer ***subtables;
	NEW(subtables, table->lookups.length);
	bool *preferExtForThisLut;
	tableid_t *subtableQuantity;
	NEW(subtableQuantity, table->lookups.length);
	NEW(preferExtForThisLut, table->lookups.length);

	size_t lastOffset = 0;
	for (tableid_t j = 0; j < table->lookups.length; j++) {
		otl_Lookup *lookup = table->lookups.items[j];
		otl_BuildHeuristics heu = getLookupHeuristics(table, lookup);
		logProgress("Building lookup %s (%u/%u)\n", lookup->name, j,
		            (uint32_t)table->lookups.length);
		subtableQuantity[j] =
		    _build_lookup(lookup, &(subtables[j]), &lastOffset, &(preferExtForThisLut[j]), heu);
	}

	size_t headerSize = 2 + 2 * table->lookups.length;
	for (tableid_t j = 0; j < table->lookups.length; j++) {
		if (subtableQuantity[j]) { headerSize += 6 + 2 * subtableQuantity[j]; }
	}
	bool useExtended = lastOffset >= 0xFF00 - headerSize;

	bk_Block *root = bk_new_Block(b16, table->lookups.length, // LookupCount
	                              bkover);
	for (tableid_t j = 0; j < table->lookups.length; j++) {
		if (!subtableQuantity[j]) {
			logNotice("Lookup %s is empty.\n", table->lookups.items[j]->name);
		}
		otl_Lookup *lookup = table->lookups.items[j];
		const bool canBeContextual = otfcc_chainingLookupIsContextualLookup(lookup);
		const bool useExtendedForIt = useExtended || preferExtForThisLut[j];
		if (useExtendedForIt) {
			logNotice("[OTFCC-fea] Using extended OpenType table layout for %s/%s.\n", tag,
			          lookup->name);
		}
		uint16_t lookupType =
		    useExtendedForIt
		        ? (lookup->type > otl_type_gpos_unknown
		               ? otl_type_gpos_extend - otl_type_gpos_unknown
		               : lookup->type > otl_type_gsub_unknown
		                     ? otl_type_gsub_extend - otl_type_gsub_unknown
		                     : 0)
		        : (lookup->type > otl_type_gpos_unknown
		               ? lookup->type - otl_type_gpos_unknown
		               : lookup->type > otl_type_gsub_unknown ? lookup->type - otl_type_gsub_unknown
		                                                      : 0) -
		              (canBeContextual ? 1 : 0);

		bk_Block *blk = bk_new_Block(b16, lookupType,          // LookupType
		                             b16, lookup->flags,       // LookupFlag
		                             b16, subtableQuantity[j], // SubTableCount
		                             bkover);

		for (tableid_t k = 0; k < subtableQuantity[j]; k++) {
			if (useExtendedForIt) {
				uint16_t extensionLookupType = (lookup->type > otl_type_gpos_unknown
				                                    ? lookup->type - otl_type_gpos_unknown
				                                    : lookup->type > otl_type_gsub_unknown
				                                          ? lookup->type - otl_type_gsub_unknown
				                                          : 0) -
				                               (canBeContextual ? 1 : 0);

				bk_Block *stub =
				    bk_new_Block(b16, 1,                                      // format
				                 b16, extensionLookupType,                    // ExtensionLookupType
				                 p32, bk_newBlockFromBuffer(subtables[j][k]), // ExtensionOffset
				                 bkover);
				bk_push(blk, p16, stub, bkover);
			} else {
				bk_push(blk, p16, bk_newBlockFromBuffer(subtables[j][k]), bkover);
			}
		}
		bk_push(blk, b16, 0, // MarkFilteringSet
		        bkover);
		bk_push(root, p16, blk, bkover);
		FREE(subtables[j]);
	}
	FREE(subtables);
	FREE(subtableQuantity);
	FREE(preferExtForThisLut);
	return root;
}

static bk_Block *writeOTLFeatures(const table_OTL *table, const otfcc_Options *options) {
	bk_Block *root = bk_new_Block(b16, table->features.length, bkover);
	for (tableid_t j = 0; j < table->features.length; j++) {
		bk_Block *fea = bk_new_Block(p16, NULL,                                     // FeatureParams
		                             b16, table->features.items[j]->lookups.length, // LookupCount
		                             bkover);
		for (tableid_t k = 0; k < table->features.items[j]->lookups.length; k++) {
			// reverse lookup
			for (tableid_t l = 0; l < table->lookups.length; l++) {
				if (table->features.items[j]->lookups.items[k] == table->lookups.items[l]) {
					bk_push(fea, b16, l, bkover);
					break;
				}
			}
		}
		bk_push(root, b32, featureNameToTag(table->features.items[j]->name), // FeatureTag
		        p16, fea,                                                    // Feature
		        bkover);
	}
	return root;
}

// script hash
typedef struct {
	sds tag;
	uint16_t lc;
	otl_LanguageSystem *dl;
	otl_LanguageSystem **ll;
	UT_hash_handle hh;
} script_stat_hash;

static tableid_t featureIndex(const otl_Feature *feature, const table_OTL *table) {
	for (tableid_t j = 0; j < table->features.length; j++)
		if (table->features.items[j] == feature) { return j; }
	return 0xFFFF;
}
static bk_Block *writeLanguage(otl_LanguageSystem *lang, const table_OTL *table) {
	if (!lang) return NULL;
	bk_Block *root =
	    bk_new_Block(p16, NULL,                                       // LookupOrder
	                 b16, featureIndex(lang->requiredFeature, table), // ReqFeatureIndex
	                 b16, lang->features.length,                      // FeatureCount
	                 bkover);
	for (tableid_t k = 0; k < lang->features.length; k++) {
		bk_push(root, b16, featureIndex(lang->features.items[k], table), bkover);
	}
	return root;
}

static bk_Block *writeScript(script_stat_hash *script, const table_OTL *table) {
	bk_Block *root = bk_new_Block(p16, writeLanguage(script->dl, table), // DefaultLangSys
	                              b16, script->lc,                       // LangSysCount
	                              bkover);

	for (tableid_t j = 0; j < script->lc; j++) {
		sds tag = sdsnewlen(script->ll[j]->name + 5, 4);

		bk_push(root, b32, featureNameToTag(tag),         // LangSysTag
		        p16, writeLanguage(script->ll[j], table), // LangSys
		        bkover);
		sdsfree(tag);
	}
	return root;
}
static bk_Block *writeOTLScriptAndLanguages(const table_OTL *table, const otfcc_Options *options) {
	script_stat_hash *h = NULL;
	for (tableid_t j = 0; j < table->languages.length; j++) {
		otl_LanguageSystem *language = table->languages.items[j];
		sds scriptTag = sdsnewlen(language->name, 4);
		bool isDefault = strncmp(language->name + 5, "DFLT", 4) == 0 ||
		                 strncmp(language->name + 5, "dflt", 4) == 0;

		script_stat_hash *s = NULL;
		HASH_FIND_STR(h, scriptTag, s);
		if (s) {
			if (isDefault) {
				s->dl = language;
			} else {
				s->lc += 1;
				s->ll[s->lc - 1] = language;
			}
			sdsfree(scriptTag);
		} else {
			NEW(s);
			s->tag = scriptTag;
			s->dl = NULL;
			NEW(s->ll, table->languages.length);
			if (isDefault) {
				s->dl = language;
				s->lc = 0;
			} else {
				s->lc = 1;
				s->ll[s->lc - 1] = language;
			}
			HASH_ADD_STR(h, tag, s);
		}
	}

	bk_Block *root = bk_new_Block(b16, HASH_COUNT(h), bkover);

	script_stat_hash *s, *tmp;
	HASH_ITER(hh, h, s, tmp) {
		bk_push(root, b32, featureNameToTag(s->tag), // ScriptTag
		        p16, writeScript(s, table),          // Script
		        bkover);
		HASH_DEL(h, s);
		sdsfree(s->tag);
		FREE(s->ll);
		FREE(s);
	}
	return root;
}

caryll_Buffer *otfcc_buildOtl(const table_OTL *table, const otfcc_Options *options,
                              const char *tag) {
	if (!table) return NULL;
	caryll_Buffer *buf;
	loggedStep("%s", tag) {
		bk_Block *lookups = writeOTLLookups(table, options, tag);
		bk_Block *features = writeOTLFeatures(table, options);
		bk_Block *languages = writeOTLScriptAndLanguages(table, options);
		bk_Block *root = bk_new_Block(b32, 0x10000,   // Version
		                              p16, languages, // ScriptList
		                              p16, features,  // FeatureList
		                              p16, lookups,   // LookupList
		                              bkover);
		buf = bk_build_Block(root);
	}
	return buf;
}
