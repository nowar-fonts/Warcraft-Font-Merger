#ifndef CARYLL_INCLUDE_TABLE_HEAD_H
#define CARYLL_INCLUDE_TABLE_HEAD_H

#include "table-common.h"

typedef struct {
	// Font header
	f16dot16 version;
	uint32_t fontRevision;
	uint32_t checkSumAdjustment;
	uint32_t magicNumber;
	uint16_t flags;
	uint16_t unitsPerEm;
	int64_t created;
	int64_t modified;
	int16_t xMin;
	int16_t yMin;
	int16_t xMax;
	int16_t yMax;
	uint16_t macStyle;
	uint16_t lowestRecPPEM;
	int16_t fontDirectoryHint;
	int16_t indexToLocFormat;
	int16_t glyphDataFormat;
} table_head;

extern caryll_RefElementInterface(table_head) table_iHead;

#endif
