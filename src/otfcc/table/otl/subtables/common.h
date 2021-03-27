#ifndef CARYLL_TABLE_OTL_SUBTABLES_COMMON_H
#define CARYLL_TABLE_OTL_SUBTABLES_COMMON_H

#include "support/util.h"
#include "bk/bkgraph.h"
#include "../../otl.h"

typedef enum {
	OTL_BH_NORMAL = 0,
	OTL_BH_GSUB_VERT = 1
} otl_BuildHeuristics;

#define checkLength(offset) if (tableLength < offset) { goto FAIL; }

#endif
