#include "otfcc/handle.h"
#include "element-impl.h"
#include "support/otfcc-alloc.h"

// default constructors
static INLINE void initHandle(otfcc_Handle *h) {
	h->state = HANDLE_STATE_EMPTY;
	h->index = 0;
	h->name = NULL;
}
static INLINE void disposeHandle(struct otfcc_Handle *h) {
	if (h->name) {
		sdsfree(h->name);
		h->name = NULL;
	}
	h->index = 0;
	h->state = HANDLE_STATE_EMPTY;
}
static void copyHandle(otfcc_Handle *dst, const otfcc_Handle *src) {
	dst->state = src->state;
	dst->index = src->index;
	if (src->name) {
		dst->name = sdsdup(src->name);
	} else {
		dst->name = NULL;
	}
}

caryll_standardValTypeFn(otfcc_Handle, initHandle, copyHandle, disposeHandle);

// custom constructors
static struct otfcc_Handle handle_fromIndex(glyphid_t id) {
	struct otfcc_Handle h = {HANDLE_STATE_INDEX, id, NULL};
	return h;
}
static struct otfcc_Handle handle_fromName(MOVE sds s) {
	struct otfcc_Handle h = {HANDLE_STATE_EMPTY, 0, NULL};
	if (s) {
		h.state = HANDLE_STATE_NAME;
		h.name = s;
	}
	return h;
}
static struct otfcc_Handle handle_fromConsolidated(glyphid_t id, sds s) {
	struct otfcc_Handle h = {HANDLE_STATE_CONSOLIDATED, id, sdsdup(s)};
	return h;
}

// consolidation
static void handle_consolidateTo(struct otfcc_Handle *h, glyphid_t id, sds name) {
	otfcc_iHandle.dispose(h);
	h->state = HANDLE_STATE_CONSOLIDATED;
	h->index = id;
	h->name = sdsdup(name);
}

const struct otfcc_HandlePackage otfcc_iHandle = {
    caryll_standardValTypeMethods(otfcc_Handle), // VT
    .fromIndex = handle_fromIndex,               // custom constructor, from index
    .fromName = handle_fromName,                 // custom constructor, from name
    .fromConsolidated = handle_fromConsolidated, // custom constructor, from consolidated
    .consolidateTo = handle_consolidateTo,
};
