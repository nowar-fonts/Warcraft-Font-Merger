#ifndef CARYLL_INCLUDE_TABLE_VDMX_H
#define CARYLL_INCLUDE_TABLE_VDMX_H

#include "table-common.h"

typedef struct {
	uint16_t yPelHeight;
	int16_t yMax;
	int16_t yMin;
} vdmx_Record;

extern caryll_ValElementInterface(vdmx_Record) vdmx_iRecord;
typedef caryll_Vector(vdmx_Record) vdmx_Group;
extern caryll_VectorInterface(vdmx_Group, vdmx_Record) vdmx_iGroup;

typedef struct {
	uint8_t bCharset;
	uint8_t xRatio;
	uint8_t yStartRatio;
	uint8_t yEndRatio;

	vdmx_Group records;
} vdmx_RatioRange;

extern caryll_ElementInterface(vdmx_RatioRange) vdmx_iRatioRange;
typedef caryll_Vector(vdmx_RatioRange) vdmx_RatioRagneList;
extern caryll_VectorInterface(vdmx_RatioRagneList, vdmx_RatioRange) vdmx_iRatioRangeList;

typedef struct {
	uint16_t version;
	vdmx_RatioRagneList ratios;
} table_VDMX;

extern caryll_RefElementInterface(table_VDMX) table_iVDMX;

#endif
