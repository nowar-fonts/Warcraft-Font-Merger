#ifndef CARYLL_VF_VV_H
#define CARYLL_VF_VV_H

#include "caryll/element.h"
#include "caryll/vector.h"
#include "otfcc/primitives.h"

extern caryll_ValElementInterface(pos_t) vq_iPosT;
typedef caryll_Vector(pos_t) VV;
extern caryll_VectorInterfaceTypeName(VV) {
	caryll_VectorInterfaceTrait(VV, pos_t);
	// Monoid instances
	VV (*neutral)(tableid_t dimensions);
}
iVV;
// extern caryll_VectorInterface(VV, pos_t) iVV;

#endif
