#include "../VDMX.h"

#include "support/util.h"

caryll_standardValType(vdmx_Record, vdmx_iRecord);

caryll_standardVectorImpl(vdmx_Group, vdmx_Record, vdmx_iRecord, vdmx_iGroup);

static void initRR(vdmx_RatioRange *rr) {
	memset(rr, 0, sizeof(*rr));
	vdmx_iGroup.init(&rr->records);
}

static void disposeRR(vdmx_RatioRange *rr) {
	vdmx_iGroup.dispose(&rr->records);
}

caryll_standardType(vdmx_RatioRange, vdmx_iRatioRange, initRR, disposeRR);
caryll_standardVectorImpl(vdmx_RatioRagneList, vdmx_RatioRange, vdmx_iRatioRange,
                          vdmx_iRatioRangeList);

static void initVDMX(table_VDMX *t) {
	vdmx_iRatioRangeList.init(&t->ratios);
}
static void disposeVDMX(table_VDMX *t) {
	vdmx_iRatioRangeList.dispose(&t->ratios);
}
caryll_standardRefType(table_VDMX, table_iVDMX, initVDMX, disposeVDMX);
