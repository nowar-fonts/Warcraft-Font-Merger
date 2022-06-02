#ifndef CARYLL_VF_AXIS_H
#define CARYLL_VF_AXIS_H

#include "caryll/element.h"
#include "caryll/vector.h"
#include "otfcc/primitives.h"

typedef struct {
	uint32_t tag;
	pos_t minValue;
	pos_t defaultValue;
	pos_t maxValue;
	uint16_t flags;
	uint16_t axisNameID;
} vf_Axis;

extern caryll_ValElementInterface(vf_Axis) vf_iAxis;
typedef caryll_Vector(vf_Axis) vf_Axes;
extern caryll_VectorInterface(vf_Axes, vf_Axis) vf_iAxes;

#endif
