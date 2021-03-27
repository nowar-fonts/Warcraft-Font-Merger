#ifndef CARYLL_INCLUDE_TABLE_GASP_H
#define CARYLL_INCLUDE_TABLE_GASP_H

#include "table-common.h"

typedef struct {
	glyphsize_t rangeMaxPPEM;
	bool dogray;
	bool gridfit;
	bool symmetric_smoothing;
	bool symmetric_gridfit;
} gasp_Record;
extern caryll_ElementInterface(gasp_Record) gasp_iRecord;
typedef caryll_Vector(gasp_Record) gasp_RecordList;
extern caryll_VectorInterface(gasp_RecordList, gasp_Record) gasp_iRecordList;

typedef struct {
	uint16_t version;
	OWNING gasp_RecordList records;
} table_gasp;

extern caryll_RefElementInterface(table_gasp) table_iGasp;

#endif
