#ifndef CARYLL_INCLUDE_TABLE_FPGM_PREP_H
#define CARYLL_INCLUDE_TABLE_FPGM_PREP_H

#include "table-common.h"

typedef struct {
	sds tag;
	uint32_t length;
	OWNING uint8_t *bytes;
} table_fpgm_prep;

extern caryll_RefElementInterface(table_fpgm_prep) table_iFpgm_prep;

#endif
