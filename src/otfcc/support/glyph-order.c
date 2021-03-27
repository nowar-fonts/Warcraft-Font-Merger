#include "util.h"
#include "otfcc/glyph-order.h"

static INLINE void initGlyphOrder(otfcc_GlyphOrder *go) {
	go->byGID = NULL;
	go->byName = NULL;
}
static INLINE void disposeGlyphOrder(otfcc_GlyphOrder *go) {
	otfcc_GlyphOrderEntry *current, *temp;
	HASH_ITER(hhID, go->byGID, current, temp) {
		if (current->name) sdsfree(current->name);
		HASH_DELETE(hhID, go->byGID, current);
		HASH_DELETE(hhName, go->byName, current);
		FREE(current);
	}
}
caryll_standardRefTypeFn(otfcc_GlyphOrder, initGlyphOrder, disposeGlyphOrder);

// Register a gid->name map
static sds otfcc_setGlyphOrderByGID(otfcc_GlyphOrder *go, glyphid_t gid, sds name) {
	otfcc_GlyphOrderEntry *s = NULL;
	HASH_FIND(hhID, go->byGID, &gid, sizeof(glyphid_t), s);
	if (s) {
		// gid is already in the order table.
		// reject this naming suggestion.
		sdsfree(name);
		return s->name;
	} else {
		otfcc_GlyphOrderEntry *t = NULL;
		HASH_FIND(hhName, go->byName, name, sdslen(name), t);
		if (t) {
			// The name is already in-use.
			sdsfree(name);
			name = sdscatprintf(sdsempty(), "$$gid%d", gid);
		}
		NEW(s);
		s->gid = gid;
		s->name = name;
		HASH_ADD(hhID, go->byGID, gid, sizeof(glyphid_t), s);
		HASH_ADD(hhName, go->byName, name[0], sdslen(s->name), s);
	}
	return name;
}

// Register a name->gid map
static bool otfcc_setGlyphOrderByName(otfcc_GlyphOrder *go, sds name, glyphid_t gid) {
	otfcc_GlyphOrderEntry *s = NULL;
	HASH_FIND(hhName, go->byName, name, sdslen(name), s);
	if (s) {
		// name is already mapped to a glyph
		// reject this naming suggestion
		return false;
	} else {
		NEW(s);
		s->gid = gid;
		s->name = name;
		HASH_ADD(hhID, go->byGID, gid, sizeof(glyphid_t), s);
		HASH_ADD(hhName, go->byName, name[0], sdslen(s->name), s);
		return true;
	}
}

static bool otfcc_gordNameAFieldShared(otfcc_GlyphOrder *go, glyphid_t gid, sds *field) {
	otfcc_GlyphOrderEntry *t;
	HASH_FIND(hhID, go->byGID, &gid, sizeof(glyphid_t), t);
	if (t != NULL) {
		*field = t->name;
		return true;
	} else {
		*field = NULL;
		return false;
	}
}

static bool otfcc_gordConsolidateHandle(otfcc_GlyphOrder *go, glyph_handle *h) {
	if (h->state == HANDLE_STATE_CONSOLIDATED) {
		otfcc_GlyphOrderEntry *t;
		HASH_FIND(hhName, go->byName, h->name, sdslen(h->name), t);
		if (t) {
			Handle.consolidateTo(h, t->gid, t->name);
			return true;
		}
		HASH_FIND(hhName, go->byGID, &(h->index), sizeof(glyphid_t), t);
		if (t) {
			Handle.consolidateTo(h, t->gid, t->name);
			return true;
		}
	} else if (h->state == HANDLE_STATE_NAME) {
		otfcc_GlyphOrderEntry *t;
		HASH_FIND(hhName, go->byName, h->name, sdslen(h->name), t);
		if (t) {
			Handle.consolidateTo(h, t->gid, t->name);
			return true;
		}
	} else if (h->state == HANDLE_STATE_INDEX) {
		sds name = NULL;
		otfcc_gordNameAFieldShared(go, h->index, &name);
		if (name) {
			Handle.consolidateTo(h, h->index, name);
			return true;
		}
	}
	return false;
}

static bool gordLookupName(otfcc_GlyphOrder *go, sds name) {
	otfcc_GlyphOrderEntry *t;
	HASH_FIND(hhName, go->byName, name, sdslen(name), t);
	if (t) return true;
	return false;
}

const struct otfcc_GlyphOrderPackage otfcc_pkgGlyphOrder = {
    caryll_standardRefTypeMethods(otfcc_GlyphOrder),  .setByGID = otfcc_setGlyphOrderByGID,
    .setByName = otfcc_setGlyphOrderByName,           .nameAField_Shared = otfcc_gordNameAFieldShared,
    .consolidateHandle = otfcc_gordConsolidateHandle, .lookupName = gordLookupName,
};
