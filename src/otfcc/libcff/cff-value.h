#ifndef CARYLL_cff_VALUE_H
#define CARYLL_cff_VALUE_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "support/util.h"

typedef enum {
	cff_OPERATOR = 1,
	CS2_OPERATOR = 1,
	cff_INTEGER = 2,
	CS2_OPERAND = 2,
	cff_DOUBLE = 3,
	CS2_FRACTION = 3
} cff_Value_Type;

typedef struct {
	cff_Value_Type t;
	union {
		int32_t i;
		double d;
	};
} cff_Value;

double cffnum(cff_Value v);

#endif
