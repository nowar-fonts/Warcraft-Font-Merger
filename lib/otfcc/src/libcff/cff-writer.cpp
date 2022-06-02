/*
  Compiler of CFF, subset and full conversion
    * note that there is no optimization in current implement
*/

#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libcff.h"

caryll_Buffer *cff_buildHeader(void) {
	return bufninit(4, 1, 0, 4, 4);
}

void cff_mergeCS2Operator(caryll_Buffer *blob, int32_t val) {
	if (val >= 0x100) {
		bufnwrite8(blob, 2, val >> 8, val & 0xff);
	} else {
		bufnwrite8(blob, 1, val & 0xff);
	}
}
void cff_mergeCS2Int(caryll_Buffer *blob, int32_t val) {
	if (val >= -1131 && val <= -108) {
		bufnwrite8(blob, 2, (uint8_t)((-108 - val) / 256 + 251), (uint8_t)((-108 - val) % 256));
	} else if (val >= -107 && val <= 107) {
		bufnwrite8(blob, 1, (uint8_t)(val + 139));
	} else if (val >= 108 && val <= 1131) {
		bufnwrite8(blob, 2, (uint8_t)((val - 108) / 256 + 247), (uint8_t)((val - 108) % 256));
	} else {
		if (val >= -32768 && val <= 32767) {
			bufnwrite8(blob, 3, 28, (uint8_t)(val >> 8), (uint8_t)((val << 8) >> 8));
		} else {
			cff_mergeCS2Int(blob, 0);
		}
	}
}
static void mergeCS2Real(caryll_Buffer *blob, double val) {
	int16_t integerPart = floor(val);
	uint16_t fractionPart = (val - integerPart) * 65536.0;
	bufnwrite8(blob, 5, 0xFF, integerPart >> 8, integerPart & 0xFF, fractionPart >> 8, fractionPart & 0xFF);
}
void cff_mergeCS2Operand(caryll_Buffer *blob, double val) {
	double intpart;
	if (modf(val, &intpart) == 0.0) {
		cff_mergeCS2Int(blob, (int32_t)intpart);
	} else {
		mergeCS2Real(blob, val);
	}
}
void cff_mergeCS2Special(caryll_Buffer *blob, uint8_t val) {
	bufwrite8(blob, val);
}

caryll_Buffer *cff_buildOffset(int32_t val) {
	return bufninit(5, 29, (val >> 24) & 0xff, (val >> 16) & 0xff, (val >> 8) & 0xff, val & 0xff);
}
