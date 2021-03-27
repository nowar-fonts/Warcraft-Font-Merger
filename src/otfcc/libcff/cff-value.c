#include "cff-value.h"

double cffnum(cff_Value val) {
	if (val.t == cff_INTEGER) return val.i;
	if (val.t == cff_DOUBLE) return val.d;
	return 0;
}
