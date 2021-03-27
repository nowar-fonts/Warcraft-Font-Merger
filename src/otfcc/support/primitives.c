#include "otfcc/primitives.h"
#include "bin-io.h"
#include <math.h>

// f2dot14 type
double otfcc_from_f2dot14(const f2dot14 x) {
	return x / 16384.0;
}
int16_t otfcc_to_f2dot14(const double x) {
	return round(x * 16384.0);
}

// F16.16 (fixed) type
double otfcc_from_fixed(const f16dot16 x) {
	return x / 65536.0;
}
f16dot16 otfcc_to_fixed(const double x) {
	return round(x * 65536.0);
}

// F16.16 arith
// Clamp: remove too-large values
static INLINE f16dot16 clamp(int64_t value) {
	int64_t tmp = value;
	if (tmp < (int64_t)f16dot16_negativeIntinity) tmp = (int64_t)f16dot16_negativeIntinity;
	if (tmp > (int64_t)f16dot16_infinity) tmp = (int64_t)f16dot16_infinity;
	return (f16dot16)tmp;
}

f16dot16 otfcc_f1616_add(f16dot16 a, f16dot16 b) {
	return a + b;
}
f16dot16 otfcc_f1616_minus(f16dot16 a, f16dot16 b) {
	return a - b;
}

f16dot16 otfcc_f1616_multiply(f16dot16 a, f16dot16 b) {
	int64_t tmp = (int64_t)a * (int64_t)b + f16dot16_k;
	f16dot16 product = clamp(tmp >> f16dot16_precision);
	return product;
}

static INLINE f16dot16 divide(int64_t a, int32_t b) {
	if (b == 0) {
		if (a < 0)
			return f16dot16_negativeIntinity;
		else
			return f16dot16_infinity;
	}

	if ((a < 0 != b < 0)) {
		a -= b / 2;
	} else {
		a += b / 2;
	}

	return (f16dot16)(clamp(a / b));
}

f16dot16 otfcc_f1616_muldiv(f16dot16 a, f16dot16 b, f16dot16 c) {
	int64_t tmp = (int64_t)a * (int64_t)b + f16dot16_k;
	return divide(tmp, c);
}
f16dot16 otfcc_f1616_divide(f16dot16 a, f16dot16 b) {
	return divide((int64_t)a << f16dot16_precision, b);
}
