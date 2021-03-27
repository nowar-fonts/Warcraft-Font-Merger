#ifndef CARYLL_INCLUDE_GLYPH_ORDER_H
#define CARYLL_INCLUDE_GLYPH_ORDER_H

#include "dep/sds.h"
#include "dep/json.h"
#include "dep/uthash.h"
#include "caryll/ownership.h"
#include "otfcc/primitives.h"
#include "otfcc/handle.h"
#include "otfcc/options.h"

typedef struct {
	glyphid_t gid;
	sds name;
	uint8_t orderType;
	uint32_t orderEntry;
	UT_hash_handle hhID;
	UT_hash_handle hhName;
} otfcc_GlyphOrderEntry;

typedef struct {
	otfcc_GlyphOrderEntry *byGID;
	otfcc_GlyphOrderEntry *byName;
} otfcc_GlyphOrder;

struct otfcc_GlyphOrderPackage {
	caryll_RT(otfcc_GlyphOrder);
	sds (*setByGID)(otfcc_GlyphOrder *go, glyphid_t gid, sds name);
	bool (*setByName)(otfcc_GlyphOrder *go, sds name, glyphid_t gid);
	bool (*nameAField_Shared)(otfcc_GlyphOrder *go, glyphid_t gid, sds *field); // return a shared name pointer
	bool (*consolidateHandle)(otfcc_GlyphOrder *go, otfcc_GlyphHandle *h);
	bool (*lookupName)(otfcc_GlyphOrder *go, sds name);
};

extern const struct otfcc_GlyphOrderPackage otfcc_pkgGlyphOrder;

#endif
