#ifndef CARYLL_INCLUDE_TABLE_OTL_H
#define CARYLL_INCLUDE_TABLE_OTL_H

#include "caryll/vector.h"
#include "table-common.h"
#include "otl/coverage.h"
#include "otl/classdef.h"

typedef enum {
	otl_type_unknown = 0,

	otl_type_gsub_unknown = 0x10,
	otl_type_gsub_single = 0x11,
	otl_type_gsub_multiple = 0x12,
	otl_type_gsub_alternate = 0x13,
	otl_type_gsub_ligature = 0x14,
	otl_type_gsub_context = 0x15,
	otl_type_gsub_chaining = 0x16,
	otl_type_gsub_extend = 0x17,
	otl_type_gsub_reverse = 0x18,

	otl_type_gpos_unknown = 0x20,
	otl_type_gpos_single = 0x21,
	otl_type_gpos_pair = 0x22,
	otl_type_gpos_cursive = 0x23,
	otl_type_gpos_markToBase = 0x24,
	otl_type_gpos_markToLigature = 0x25,
	otl_type_gpos_markToMark = 0x26,
	otl_type_gpos_context = 0x27,
	otl_type_gpos_chaining = 0x28,
	otl_type_gpos_extend = 0x29
} otl_LookupType;

typedef union _otl_subtable otl_Subtable;
typedef struct _otl_lookup otl_Lookup;

typedef struct {
	pos_t dx;
	pos_t dy;
	pos_t dWidth;
	pos_t dHeight;
} otl_PositionValue;

// GSUB subtable formats
typedef struct {
	OWNING otfcc_GlyphHandle from;
	OWNING otfcc_GlyphHandle to;
} otl_GsubSingleEntry;
typedef caryll_Vector(otl_GsubSingleEntry) subtable_gsub_single;
extern caryll_VectorInterface(subtable_gsub_single, otl_GsubSingleEntry) iSubtable_gsub_single;

typedef struct {
	OWNING otfcc_GlyphHandle from;
	OWNING otl_Coverage *to;
} otl_GsubMultiEntry;
typedef caryll_Vector(otl_GsubMultiEntry) subtable_gsub_multi;
extern caryll_VectorInterface(subtable_gsub_multi, otl_GsubMultiEntry) iSubtable_gsub_multi;

typedef struct {
	OWNING otl_Coverage *from;
	OWNING otfcc_GlyphHandle to;
} otl_GsubLigatureEntry;
typedef caryll_Vector(otl_GsubLigatureEntry) subtable_gsub_ligature;
extern caryll_VectorInterface(subtable_gsub_ligature,
                              otl_GsubLigatureEntry) iSubtable_gsub_ligature;

typedef enum {
	otl_chaining_canonical =
	    0, // The canonical form of chaining contextual substitution, one rule per subtable.
	otl_chaining_poly = 1, // The multi-rule form, right after reading OTF. N rule per subtable.
	otl_chaining_classified =
	    2 // The classified intermediate form, for building TTF with compression.
	      // N rules, has classdefs, and coverage GID interpreted as class number.
} otl_chaining_type;

typedef struct {
	tableid_t index;
	otfcc_LookupHandle lookup;
} otl_ChainLookupApplication;
typedef struct {
	tableid_t matchCount;
	tableid_t inputBegins;
	tableid_t inputEnds;
	OWNING otl_Coverage **match;
	tableid_t applyCount;
	OWNING otl_ChainLookupApplication *apply;
} otl_ChainingRule;
typedef struct {
	otl_chaining_type type;
	union {
		otl_ChainingRule rule; // for type = otl_chaining_canonical
		struct {               // for type = otl_chaining_poly or otl_chaining_classified
			tableid_t rulesCount;
			OWNING otl_ChainingRule **rules;
			OWNING otl_ClassDef *bc;
			OWNING otl_ClassDef *ic;
			OWNING otl_ClassDef *fc;
		};
	};
} subtable_chaining;
extern caryll_RefElementInterface(subtable_chaining) iSubtable_chaining;

typedef struct {
	tableid_t matchCount;
	tableid_t inputIndex;
	OWNING otl_Coverage **match;
	OWNING otl_Coverage *to;
} subtable_gsub_reverse;
extern caryll_RefElementInterface(subtable_gsub_reverse) iSubtable_gsub_reverse;

// GPOS subtable formats
typedef struct {
	OWNING otfcc_GlyphHandle target;
	OWNING otl_PositionValue value;
} otl_GposSingleEntry;
typedef caryll_Vector(otl_GposSingleEntry) subtable_gpos_single;
extern caryll_VectorInterface(subtable_gpos_single, otl_GposSingleEntry) iSubtable_gpos_single;

typedef struct {
	bool present;
	pos_t x;
	pos_t y;
} otl_Anchor;

typedef struct {
	OWNING otl_ClassDef *first;
	OWNING otl_ClassDef *second;
	OWNING otl_PositionValue **firstValues;
	OWNING otl_PositionValue **secondValues;
} subtable_gpos_pair;
extern caryll_RefElementInterface(subtable_gpos_pair) iSubtable_gpos_pair;

typedef struct {
	OWNING otfcc_GlyphHandle target;
	OWNING otl_Anchor enter;
	OWNING otl_Anchor exit;
} otl_GposCursiveEntry;
typedef caryll_Vector(otl_GposCursiveEntry) subtable_gpos_cursive;
extern caryll_VectorInterface(subtable_gpos_cursive, otl_GposCursiveEntry) iSubtable_gpos_cursive;

typedef struct {
	OWNING otfcc_GlyphHandle glyph;
	glyphclass_t markClass;
	otl_Anchor anchor;
} otl_MarkRecord;
typedef caryll_Vector(otl_MarkRecord) otl_MarkArray;
extern caryll_VectorInterface(otl_MarkArray, otl_MarkRecord) otl_iMarkArray;

typedef struct {
	OWNING otfcc_GlyphHandle glyph;
	OWNING otl_Anchor *anchors;
} otl_BaseRecord;
typedef caryll_Vector(otl_BaseRecord) otl_BaseArray;
extern caryll_VectorInterface(otl_BaseArray, otl_BaseRecord) otl_iBaseArray;

typedef struct {
	glyphclass_t classCount;
	OWNING otl_MarkArray markArray;
	otl_BaseArray baseArray;
} subtable_gpos_markToSingle;
extern caryll_RefElementInterface(subtable_gpos_markToSingle) iSubtable_gpos_markToSingle;

typedef struct {
	OWNING otfcc_GlyphHandle glyph;
	glyphid_t componentCount;
	OWNING otl_Anchor **anchors;
} otl_LigatureBaseRecord;
typedef caryll_Vector(otl_LigatureBaseRecord) otl_LigatureArray;
extern caryll_VectorInterface(otl_LigatureArray, otl_LigatureBaseRecord) otl_iLigatureArray;

typedef struct {
	glyphclass_t classCount;
	OWNING otl_MarkArray markArray;
	otl_LigatureArray ligArray;
} subtable_gpos_markToLigature;
extern caryll_RefElementInterface(subtable_gpos_markToLigature) iSubtable_gpos_markToLigature;

typedef struct {
	otl_LookupType type;
	otl_Subtable *subtable;
} subtable_extend;

typedef union _otl_subtable {
	subtable_gsub_single gsub_single;
	subtable_gsub_multi gsub_multi;
	subtable_gsub_ligature gsub_ligature;
	subtable_chaining chaining;
	subtable_gsub_reverse gsub_reverse;
	subtable_gpos_single gpos_single;
	subtable_gpos_pair gpos_pair;
	subtable_gpos_cursive gpos_cursive;
	subtable_gpos_markToSingle gpos_markToSingle;
	subtable_gpos_markToLigature gpos_markToLigature;
	subtable_extend extend;
} otl_Subtable;

typedef otl_Subtable *otl_SubtablePtr;
typedef caryll_Vector(otl_SubtablePtr) otl_SubtableList;
extern caryll_VectorInterfaceTypeName(otl_SubtableList) {
	caryll_VectorInterfaceTrait(otl_SubtableList, otl_SubtablePtr);
	void (*disposeDependent)(MODIFY otl_SubtableList *, const otl_Lookup *);
}
otl_iSubtableList;

struct _otl_lookup {
	sds name;
	otl_LookupType type;
	uint32_t _offset;
	uint16_t flags;
	OWNING otl_SubtableList subtables;
};

// owning lookup list
typedef OWNING otl_Lookup *otl_LookupPtr;
extern caryll_ElementInterface(otl_LookupPtr) otl_iLookupPtr;
typedef caryll_Vector(otl_LookupPtr) otl_LookupList;
extern caryll_VectorInterface(otl_LookupList, otl_LookupPtr) otl_iLookupList;

// observe lookup list
typedef OBSERVE otl_Lookup *otl_LookupRef;
extern caryll_ElementInterface(otl_LookupRef) otl_iLookupRef;
typedef caryll_Vector(otl_LookupRef) otl_LookupRefList;
extern caryll_VectorInterface(otl_LookupRefList, otl_LookupRef) otl_iLookupRefList;

typedef struct {
	sds name;
	OWNING otl_LookupRefList lookups;
} otl_Feature;
// owning feature list
typedef OWNING otl_Feature *otl_FeaturePtr;
extern caryll_ElementInterface(otl_FeaturePtr) otl_iFeaturePtr;
typedef caryll_Vector(otl_FeaturePtr) otl_FeatureList;
extern caryll_VectorInterface(otl_FeatureList, otl_FeaturePtr) otl_iFeatureList;
// observe feature list
typedef OBSERVE otl_Feature *otl_FeatureRef;
extern caryll_ElementInterface(otl_FeatureRef) otl_iFeatureRef;
typedef caryll_Vector(otl_FeatureRef) otl_FeatureRefList;
extern caryll_VectorInterface(otl_FeatureRefList, otl_FeatureRef) otl_iFeatureRefList;

typedef struct {
	sds name;
	OWNING otl_FeatureRef requiredFeature;
	OWNING otl_FeatureRefList features;
} otl_LanguageSystem;
typedef otl_LanguageSystem *otl_LanguageSystemPtr;
extern caryll_ElementInterface(otl_LanguageSystemPtr) otl_iLanguageSystem;
typedef caryll_Vector(otl_LanguageSystemPtr) otl_LangSystemList;
extern caryll_VectorInterface(otl_LangSystemList, otl_LanguageSystemPtr) otl_iLangSystemList;

typedef struct {
	otl_LookupList lookups;
	otl_FeatureList features;
	otl_LangSystemList languages;
} table_OTL;
extern caryll_RefElementInterface(table_OTL) table_iOTL;

#endif
