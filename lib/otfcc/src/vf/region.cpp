#include "otfcc/vf/axis.h"
#include "support/util.h"

vq_Region *vq_createRegion(shapeid_t dimensions) {
	vq_Region *r;
	NEW_CLEAN_S(r, (VQ_REGION_SIZE(dimensions)));
	r->dimensions = dimensions;
	return r;
}

void vq_deleteRegion(MOVE vq_Region *region) {
	FREE(region);
}

vq_Region *vq_copyRegion(const vq_Region *region) {
	vq_Region *dst = vq_createRegion(region->dimensions);
	memcpy(dst, region, VQ_REGION_SIZE(region->dimensions));
	return dst;
}

int vq_compareRegion(const vq_Region *a, const vq_Region *b) {
	if (a->dimensions < b->dimensions) return -1;
	if (a->dimensions > b->dimensions) return 1;
	return strncmp((const char *)a, (const char *)b, VQ_REGION_SIZE(a->dimensions));
}

bool vq_AxisSpanIsOne(const vq_AxisSpan *s) {
	const pos_t a = s->start;
	const pos_t p = s->peak;
	const pos_t z = s->end;
	return a > p || p > z || (a < 0 && z > 0 && p != 0) || (p == 0);
}

static pos_t INLINE weightAxisRegion(const vq_AxisSpan *as, const pos_t x) {
	const pos_t a = as->start;
	const pos_t p = as->peak;
	const pos_t z = as->end;
	if (a > p || p > z) {
		return 1;
	} else if (a < 0 && z > 0 && p != 0) {
		return 1;
	} else if (p == 0) {
		return 1;
	} else if (x < a || x > z) {
		return 0;
	} else if (x == p) {
		return 1;
	} else if (x < p) {
		return (x - a) / (p - a);
	} else {
		return (z - x) / (z - p);
	}
}
pos_t vqRegionGetWeight(const vq_Region *r, const VV *v) {
	pos_t w = 1;
	for (size_t j = 0; j < r->dimensions && v->length; j++) {
		w *= weightAxisRegion(&r->spans[j], v->items[j]);
	}
	return w;
}

void vq_showRegion(const vq_Region *r) {}
