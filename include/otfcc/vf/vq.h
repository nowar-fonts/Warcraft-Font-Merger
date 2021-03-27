#ifndef CARYLL_VF_FUNCTIONAL_H
#define CARYLL_VF_FUNCTIONAL_H

#include "caryll/ownership.h"
#include "caryll/element.h"
#include "caryll/vector.h"
#include "otfcc/primitives.h"
#include "otfcc/handle.h"

#include "region.h"

typedef enum { VQ_STILL = 0, VQ_DELTA = 1 } VQSegType;
typedef struct {
	VQSegType type;
	union {
		pos_t still;
		struct {
			pos_t quantity;
			bool touched;
			const vq_Region *region; // non-owning : they are in FVAR
		} delta;
	} val;
} vq_Segment;

extern caryll_ElementInterfaceOf(vq_Segment) {
	caryll_VT(vq_Segment);
	caryll_Show(vq_Segment);
	caryll_Ord(vq_Segment);
	vq_Segment (*createStill)(pos_t x);
	vq_Segment (*createDelta)(pos_t delta, vq_Region * region);
}
vq_iSegment;
typedef caryll_Vector(vq_Segment) vq_SegList;
extern caryll_VectorInterface(vq_SegList, vq_Segment) vq_iSegList;

// VQ
typedef struct {
	pos_t kernel;
	vq_SegList shift;
} VQ;
extern caryll_VectorInterfaceTypeName(VQ) {
	caryll_VT(VQ);
	caryll_Module(VQ, scale_t); // VQ forms a module (vector space)
	caryll_Ord(VQ);             // VQs are comparable
	caryll_Show(VQ);
	// Getting still
	pos_t (*getStill)(const VQ v);
	// Creating still
	VQ (*createStill)(pos_t x);
	// Being still
	bool (*isStill)(const VQ v);
	// Being zero
	bool (*isZero)(const VQ v, const pos_t err);
	// util functions
	// point linear transform
	VQ (*pointLinearTfm)(const VQ ax, pos_t a, const VQ x, pos_t b, const VQ y);
	void (*addDelta)(MODIFY VQ * v, const bool touched, const vq_Region *const r,
	                 const pos_t quantity);
}
iVQ;
#endif
