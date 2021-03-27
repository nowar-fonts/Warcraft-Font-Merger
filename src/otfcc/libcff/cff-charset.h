#ifndef CARYLL_cff_CHARSET_H
#define CARYLL_cff_CHARSET_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "support/util.h"
#include "cff-util.h"
#include "cff-value.h"

enum {
	cff_CHARSET_ISOADOBE = 0,
	cff_CHARSET_UNSPECED = 0,
	cff_CHARSET_EXPERT = 1,
	cff_CHARSET_EXPERTSUBSET = 2,
	cff_CHARSET_FORMAT0 = 3,
	cff_CHARSET_FORMAT1 = 4,
	cff_CHARSET_FORMAT2 = 5,
};

// CFF Charset Structures
typedef struct {
	uint8_t format;
	uint16_t *glyph;
} cff_CharsetFormat0;

typedef struct {
	uint16_t first;
	uint8_t nleft;
} cff_CharsetRangeFormat1;

typedef struct {
	uint8_t format;
	cff_CharsetRangeFormat1 *range1;
} cff_CharsetFormat1;

typedef struct {
	uint16_t first;
	uint16_t nleft;
} cff_CharsetRangeFormat2;

typedef struct {
	uint8_t format;
	cff_CharsetRangeFormat2 *range2;
} cff_CharsetFormat2;

typedef struct {
	uint32_t t;
	uint32_t s; // size
	union {
		cff_CharsetFormat0 f0;
		cff_CharsetFormat1 f1;
		cff_CharsetFormat2 f2;
	};
} cff_Charset;

void cff_close_Charset(cff_Charset cset);
void cff_extract_Charset(uint8_t *data, int32_t offset, uint16_t nchars, cff_Charset *charsets);
caryll_Buffer *cff_build_Charset(cff_Charset cset);

#endif
