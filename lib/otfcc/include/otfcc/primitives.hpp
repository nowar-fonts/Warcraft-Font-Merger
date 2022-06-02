#pragma once

#include <bits/stdint-intn.h>
#include <cfloat>
#include <cmath>
#include <cstdint>

namespace otfcc {

// 2.14 Fixed number, representing a value between [-1, 1].
struct f2dot14 {
	int16_t repr;

	f2dot14() : f2dot14(0) {}
	f2dot14(double value) : repr(round(value * 16384.0)) {}
	operator double() { return repr / 16384.0; }
};

// 16.16 Fixed number, usually used by intermediate coordiantes of a font.
// To deal with implicit deltas in GVAR we must be very careful about it.
// Arithmetic operators saturate towards positive or negative infinity.
// Infinity values short circuit expressions.
struct fixed {
	int32_t repr;

	fixed() : fixed(0) {}
	fixed(double value) : repr(round(value * 65536)) {}
	operator double() { return repr / 65536.0; }

	static constexpr int precision = 16;
	static constexpr int32_t k = 1 << (precision - 1);
	static constexpr int32_t inf = 0x7fffffff;
	static constexpr int32_t ninf = 0x80000000;

	static fixed _from_repr(int32_t repr) { return reinterpret_cast<fixed &>(repr); }
	static int32_t _clamp(int64_t value) {
		return value < ninf ? ninf : (value > inf ? inf : value);
	}

	fixed operator+(fixed rhs) { return _from_repr(this->repr + rhs.repr); }
	fixed operator-(fixed rhs) { return _from_repr(this->repr - rhs.repr); }
	fixed operator*(fixed rhs) {
		int64_t tmp = int64_t(this->repr) * int64_t(rhs.repr) + k;
		return _from_repr(_clamp(tmp >> precision));
	}

	static fixed _divide_helper(int64_t a, int32_t b) {
		if (b == 0)
			return a < 0 ? ninf : inf;
		if ((a < 0) != (b < 0))
			a -= b / 2;
		else
			a += b / 2;
		return _from_repr(_clamp(a / b));
	}

	fixed operator/(fixed rhs) {
		return _divide_helper(int64_t(this->repr) << precision, rhs.repr);
	}

	static fixed muldiv(fixed a, fixed b, fixed c) {
		int64_t tmp = int64_t(a.repr) * int64_t(b.repr) + k;
		return _divide_helper(tmp, c.repr);
	}
};

typedef uint16_t glyphid_t;    // Glyph index
typedef uint16_t glyphclass_t; // Glyph class
typedef uint16_t glyphsize_t;  // GASP glyph size
typedef uint16_t tableid_t;    // Table/Font structure index
typedef uint16_t colorid_t;    // Color index
typedef uint16_t shapeid_t;    // Shape index
typedef uint16_t cffsid_t;     // CFF/CFF2 String index
typedef uint32_t arity_t;      // CFF Arity/Stack depth
typedef uint32_t unicode_t;    // Unicode

typedef double pos_t;   // Position
typedef double scale_t; // transform scaling
constexpr pos_t POS_MAX = FLT_MAX;
constexpr pos_t POS_MIN = FLT_MIN;

typedef double length_t; // Length

} // namespace otfcc
