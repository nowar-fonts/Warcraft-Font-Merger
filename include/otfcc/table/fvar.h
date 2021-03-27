#ifndef CARYLL_INCLUDE_TABLE_FVAR_H
#define CARYLL_INCLUDE_TABLE_FVAR_H

#include "table-common.h"
#include "otfcc/vf/vf.h"

// vf_Axis and vf_Axes are defined in vf/vf.h
// fvar_Instance is defined below
typedef struct {
	uint16_t subfamilyNameID;
	uint16_t flags;
	VV coordinates;
	uint16_t postScriptNameID;
} fvar_Instance;
extern caryll_ElementInterface(fvar_Instance) fvar_iInstance;
typedef caryll_Vector(fvar_Instance) fvar_InstanceList;
extern caryll_VectorInterface(fvar_InstanceList, fvar_Instance) fvar_iInstanceList;

typedef struct {
	sds name;
	vq_Region *region;
	UT_hash_handle hh;
} fvar_Master;

typedef struct {
	uint16_t majorVersion;
	uint16_t minorVersion;
	vf_Axes axes;
	fvar_InstanceList instances;
	fvar_Master *masters;
} table_fvar;

extern caryll_ElementInterfaceOf(table_fvar) {
	caryll_RT(table_fvar);
	const vq_Region *(*registerRegion)(table_fvar * fvar, MOVE vq_Region * region);
	const fvar_Master *(*findMasterByRegion)(const table_fvar *fvar, const vq_Region *region);
}
table_iFvar;

#endif
