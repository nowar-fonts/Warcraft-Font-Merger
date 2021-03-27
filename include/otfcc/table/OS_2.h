#ifndef CARYLL_INCLUDE_TABLE_OS_2_H
#define CARYLL_INCLUDE_TABLE_OS_2_H

#include "table-common.h"

typedef struct {
	// OS/2 and Windows specific metrics
	uint16_t version;
	int16_t xAvgCharWidth;
	uint16_t usWeightClass;
	uint16_t usWidthClass;
	uint16_t fsType;
	int16_t ySubscriptXSize;
	int16_t ySubscriptYSize;
	int16_t ySubscriptXOffset;
	int16_t ySubscriptYOffset;
	int16_t ySupscriptXSize;
	int16_t ySupscriptYSize;
	int16_t ySupscriptXOffset;
	int16_t ySupscriptYOffset;
	int16_t yStrikeoutSize;
	int16_t yStrikeoutPosition;
	int16_t sFamilyClass;
	uint8_t panose[10];
	uint32_t ulUnicodeRange1;
	uint32_t ulUnicodeRange2;
	uint32_t ulUnicodeRange3;
	uint32_t ulUnicodeRange4;
	uint8_t achVendID[4];
	uint16_t fsSelection;
	uint16_t usFirstCharIndex;
	uint16_t usLastCharIndex;
	int16_t sTypoAscender;
	int16_t sTypoDescender;
	int16_t sTypoLineGap;
	uint16_t usWinAscent;
	uint16_t usWinDescent;
	uint32_t ulCodePageRange1;
	uint32_t ulCodePageRange2;
	int16_t sxHeight;
	int16_t sCapHeight;
	uint16_t usDefaultChar;
	uint16_t usBreakChar;
	uint16_t usMaxContext;
	uint16_t usLowerOpticalPointSize;
	uint16_t usUpperOpticalPointSize;
} table_OS_2;
extern caryll_RefElementInterface(table_OS_2) table_iOS_2;

#endif
