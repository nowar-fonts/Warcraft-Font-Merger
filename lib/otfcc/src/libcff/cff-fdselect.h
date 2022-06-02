#ifndef CARYLL_cff_FDSELECT_H
#define CARYLL_cff_FDSELECT_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "support/util.h"
#include "cff-util.h"
#include "cff-value.h"

enum {
	cff_FDSELECT_FORMAT0,
	cff_FDSELECT_FORMAT3,
	cff_FDSELECT_UNSPECED,
};

typedef struct {
	uint8_t format;
	uint8_t *fds;
} cff_FDSelectFormat0;

typedef struct {
	uint16_t first;
	uint8_t fd;
} cff_FDSelectRangeFormat3;

typedef struct {
	uint8_t format;
	uint16_t nranges;
	cff_FDSelectRangeFormat3 *range3;
	uint16_t sentinel;
} cff_FDSelectFormat3;

typedef struct {
	uint32_t t;
	uint32_t s;
	union {
		cff_FDSelectFormat0 f0;
		cff_FDSelectFormat3 f3;
	};
} cff_FDSelect;

void cff_close_FDSelect(cff_FDSelect fds);
void cff_extract_FDSelect(uint8_t *data, int32_t offset, uint16_t nchars, cff_FDSelect *fdselect);
caryll_Buffer *cff_build_FDSelect(cff_FDSelect fd);

#endif
