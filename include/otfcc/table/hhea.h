#ifndef CARYLL_INCLUDE_TABLE_HHEA_H
#define CARYLL_INCLUDE_TABLE_HHEA_H

#include "table-common.h"

typedef struct {
	// Horizontal header
	f16dot16 version;
	int16_t ascender;
	int16_t descender;
	int16_t lineGap;
	uint16_t advanceWidthMax;
	int16_t minLeftSideBearing;
	int16_t minRightSideBearing;
	int16_t xMaxExtent;
	int16_t caretSlopeRise;
	int16_t caretSlopeRun;
	int16_t caretOffset;
	int16_t reserved[4];
	int16_t metricDataFormat;
	uint16_t numberOfMetrics;
} table_hhea;
extern caryll_RefElementInterface(table_hhea) table_iHhea;

#endif
