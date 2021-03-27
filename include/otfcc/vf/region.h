#ifndef CARYLL_VF_REGION_H
#define CARYLL_VF_REGION_H

#include "caryll/element.h"
#include "caryll/vector.h"
#include "otfcc/primitives.h"

#include "vv.h"

typedef struct {
	pos_t start;
	pos_t peak;
	pos_t end;
} vq_AxisSpan;

bool vq_AxisSpanIsOne(const vq_AxisSpan *a);

typedef struct {
	shapeid_t dimensions;
	vq_AxisSpan spans[];
} vq_Region;

vq_Region *vq_createRegion(shapeid_t dimensions);
void vq_deleteRegion(MOVE vq_Region *region);
vq_Region *vq_copyRegion(const vq_Region *region);

int vq_compareRegion(const vq_Region *a, const vq_Region *b);
pos_t vq_regionGetWeight(const vq_Region *r, const VV *v);
void vq_showRegion(const vq_Region *r);

// function macros
#define VQ_REGION_SIZE(n) (sizeof(vq_Region) + sizeof(vq_AxisSpan) * (n))

#endif
