#include "otfcc/vf/vq.h"
#include "support/util.h"

// Variation vector
caryll_standardValType(pos_t, vq_iPosT);
caryll_VectorImplFunctions(VV, pos_t, vq_iPosT);

static VV createNeutralVV(tableid_t dimensions) {
	VV vv;
	iVV.initN(&vv, dimensions);
	for (tableid_t j = 0; j < dimensions; j++) {
		vv.items[j] = 0;
	}
	return vv;
};
caryll_VectorInterfaceTypeName(VV) iVV = {
    caryll_VectorImplAssignments(VV, pos_t, vq_iPosT),
    .neutral = createNeutralVV,
};

// VQS
static INLINE void initVQSegment(vq_Segment *vqs) {
	vqs->type = VQ_STILL;
	vqs->val.still = 0;
}
static INLINE void copyVQSegment(vq_Segment *dst, const vq_Segment *src) {
	dst->type = src->type;
	switch (dst->type) {
		case VQ_STILL:
			dst->val.still = src->val.still;
			break;
		case VQ_DELTA:
			dst->val.delta.quantity = src->val.delta.quantity;
			dst->val.delta.region = src->val.delta.region;
	}
}
static INLINE void disposeVQSegment(vq_Segment *vqs) {
	switch (vqs->type) {
		case VQ_DELTA:
			break;
		default:;
	}
	initVQSegment(vqs);
}

caryll_standardValTypeFn(vq_Segment, initVQSegment, copyVQSegment, disposeVQSegment);
static vq_Segment vqsCreateStill(pos_t x) {
	vq_Segment vqs;
	vq_iSegment.init(&vqs);
	vqs.val.still = x;
	return vqs;
}
static vq_Segment vqsCreateDelta(pos_t delta, vq_Region *region) {
	vq_Segment vqs;
	vq_iSegment.init(&vqs);
	vqs.type = VQ_DELTA;
	vqs.val.delta.quantity = delta;
	vqs.val.delta.region = region;
	return vqs;
}

static int vqsCompare(const vq_Segment a, const vq_Segment b) {
	if (a.type < b.type) return -1;
	if (a.type > b.type) return 1;
	switch (a.type) {
		case VQ_STILL: {
			if (a.val.still < b.val.still) return -1;
			if (a.val.still > b.val.still) return 1;
			return 0;
		}
		case VQ_DELTA: {
			int vqrc = vq_compareRegion(a.val.delta.region, b.val.delta.region);
			if (vqrc) return vqrc;
			if (a.val.delta.quantity < b.val.delta.quantity) return -1;
			if (a.val.delta.quantity > b.val.delta.quantity) return 1;
			return 0;
		}
	}
}
caryll_OrdEqFns(vq_Segment, vqsCompare);
static void showVQS(const vq_Segment x) {
	switch (x.type) {
		case VQ_STILL:
			fprintf(stderr, "%g", x.val.still);
			return;
		case VQ_DELTA:
			fprintf(stderr, "{%g%s", x.val.delta.quantity, x.val.delta.touched ? " " : "* ");
			vq_showRegion(x.val.delta.region);
			fprintf(stderr, "}\n");
			return;

		default:;
	}
}
caryll_ShowFns(vq_Segment, showVQS);
caryll_ElementInterfaceOf(vq_Segment) vq_iSegment = {
    caryll_standardValTypeMethods(vq_Segment),
    caryll_OrdEqAssigns(vq_Segment), // Ord and Eq
    caryll_ShowAssigns(vq_Segment),  // Show
    // creating instances
    .createStill = vqsCreateStill,
    .createDelta = vqsCreateDelta,
};

caryll_standardVectorImpl(vq_SegList, vq_Segment, vq_iSegment, vq_iSegList);

// Monoid

static INLINE void vqInit(VQ *a) {
	a->kernel = 0;
	vq_iSegList.init(&a->shift);
}
static INLINE void vqCopy(VQ *a, const VQ *b) {
	a->kernel = b->kernel;
	vq_iSegList.copy(&a->shift, &b->shift);
}
static INLINE void vqDispose(VQ *a) {
	a->kernel = 0;
	vq_iSegList.dispose(&a->shift);
}

caryll_standardValTypeFn(VQ, vqInit, vqCopy, vqDispose);
static VQ vqNeutral() {
	return iVQ.createStill(0);
}
static bool vqsCompatible(const vq_Segment a, const vq_Segment b) {
	if (a.type != b.type) return false;
	switch (a.type) {
		case VQ_STILL:
			return true;
		case VQ_DELTA:
			return 0 == vq_compareRegion(a.val.delta.region, b.val.delta.region);
	}
}
static void simplifyVq(MODIFY VQ *x) {
	if (!x->shift.length) return;
	vq_iSegList.sort(&x->shift, vq_iSegment.compareRef);
	size_t k = 0;
	for (size_t j = 1; j < x->shift.length; j++) {
		if (vqsCompatible(x->shift.items[k], x->shift.items[j])) {
			switch (x->shift.items[k].type) {
				case VQ_STILL:
					x->shift.items[k].val.still += x->shift.items[j].val.still;
					break;
				case VQ_DELTA:
					x->shift.items[k].val.delta.quantity += x->shift.items[j].val.delta.quantity;
					break;
			}
			vq_iSegment.dispose(&x->shift.items[j]);
		} else {
			x->shift.items[k] = x->shift.items[j];
			k++;
		}
	}
	x->shift.length = k + 1;
}
static void vqInplacePlus(MODIFY VQ *a, const VQ b) {
	a->kernel += b.kernel;
	for (size_t p = 0; p < b.shift.length; p++) {
		vq_Segment *k = &b.shift.items[p];
		if (k->type == VQ_STILL) {
			a->kernel += k->val.still;
		} else {
			vq_Segment s;
			vq_iSegment.copy(&s, k);
			vq_iSegList.push(&a->shift, s);
		}
	}
	simplifyVq(a);
}

caryll_MonoidFns(VQ, vqNeutral, vqInplacePlus);

// Module
static void vqInplaceScale(MODIFY VQ *a, pos_t b) {
	a->kernel *= b;
	for (size_t j = 0; j < a->shift.length; j++) {
		vq_Segment *s = &a->shift.items[j];
		switch (s->type) {
			case VQ_STILL:
				s->val.still *= b;
				break;
			case VQ_DELTA:
				s->val.delta.quantity *= b;
				break;
		}
	}
}

// Group
static void vqInplaceNegate(MODIFY VQ *a) {
	vqInplaceScale(a, -1);
}

caryll_GroupFns(VQ, vqInplaceNegate);
caryll_ModuleFns(VQ, pos_t, vqInplaceScale);

// Ord
static int vqCompare(const VQ a, const VQ b) {
	if (a.shift.length < b.shift.length) return -1;
	if (a.shift.length > b.shift.length) return 1;
	for (size_t j = 0; j < a.shift.length; j++) {
		int cr = vqsCompare(a.shift.items[j], b.shift.items[j]);
		if (cr) return cr;
	}
	return a.kernel - b.kernel;
}
caryll_OrdEqFns(VQ, vqCompare);

// Show
static void showVQ(const VQ x) {
	fprintf(stderr, "%g + {", x.kernel);
	for (size_t j = 0; j < x.shift.length; j++) {
		if (j) fprintf(stderr, " ");
		vq_iSegment.show(x.shift.items[j]);
	}
	fprintf(stderr, "}\n");
}
caryll_ShowFns(VQ, showVQ);

// Still instances
static pos_t vqGetStill(const VQ v) {
	pos_t result = v.kernel;
	for (size_t j = 0; j < v.shift.length; j++) {
		switch (v.shift.items[j].type) {
			case VQ_STILL:
				result += v.shift.items[j].val.still;
			default:;
		}
	}
	return result;
}
static VQ vqCreateStill(pos_t x) {
	VQ vq;
	iVQ.init(&vq);
	vq.kernel = x;
	return vq;
}
static bool vqIsStill(const VQ v) {
	for (size_t j = 0; j < v.shift.length; j++) {
		switch (v.shift.items[j].type) {
			case VQ_STILL:
				break;
			default:
				return false;
		}
	}
	return true;
}
static bool vqIsZero(const VQ v, const pos_t err) {
	return vqIsStill(v) && fabs(vqGetStill(v)) < err;
}
static void vqAddDelta(MODIFY VQ *v, const bool touched, const vq_Region *const r,
                       const pos_t quantity) {
	if (!quantity) return;
	vq_Segment nudge;
	nudge.type = VQ_DELTA;
	nudge.val.delta.region = r;
	nudge.val.delta.touched = touched;
	nudge.val.delta.quantity = quantity;
	vq_iSegList.push(&v->shift, nudge);
}

// pointLinearTfm
static VQ vqPointLinearTfm(const VQ ax, pos_t a, const VQ x, pos_t b, const VQ y) {
	VQ targetX = iVQ.dup(ax);
	iVQ.inplacePlusScale(&targetX, a, x);
	iVQ.inplacePlusScale(&targetX, b, y);
	return targetX;
}

caryll_VectorInterfaceTypeName(VQ) iVQ = {
    caryll_standardValTypeMethods(VQ),
    .getStill = vqGetStill,
    .createStill = vqCreateStill,
    .isStill = vqIsStill,
    .isZero = vqIsZero,
    caryll_MonoidAssigns(VQ),           // Monoid
    caryll_GroupAssigns(VQ),            // Group
    caryll_ModuleAssigns(VQ),           // Module
    caryll_OrdEqAssigns(VQ),            // Eq-Ord
    caryll_ShowAssigns(VQ),             // Show
    .pointLinearTfm = vqPointLinearTfm, // pointLinearTfm
    .addDelta = vqAddDelta              // addDelta
};
