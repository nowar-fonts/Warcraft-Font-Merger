#ifndef CARYLL_INCLUDE_TABLE_VHEA_H
#define CARYLL_INCLUDE_TABLE_VHEA_H

#include "table-common.h"

typedef struct {
	f16dot16 version;
	int16_t ascent;
	int16_t descent;
	int16_t lineGap;
	int16_t advanceHeightMax;
	int16_t minTop;
	int16_t minBottom;
	int16_t yMaxExtent;
	int16_t caretSlopeRise;
	int16_t caretSlopeRun;
	int16_t caretOffset;
	int16_t dummy0;
	int16_t dummy1;
	int16_t dummy2;
	int16_t dummy3;
	int16_t metricDataFormat;
	uint16_t numOfLongVerMetrics;
} table_vhea;

extern caryll_RefElementInterface(table_vhea) table_iVhea;

#endif
