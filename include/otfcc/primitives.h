#ifndef CARYLL_INCLUDE_OTFCC_PRIMITIVES_H
#define CARYLL_INCLUDE_OTFCC_PRIMITIVES_H

#include <stdint.h>
#include <stdbool.h>
#include <float.h>

typedef int16_t f2dot14;  // 2.14 Fixed number, representing a value between [-1, 1].
typedef int32_t f16dot16; // 16.16 Fixed number, usually used by intermediate coordiantes of a font.
                          // To deal with implicit deltas in GVAR we must be very careful about it.
                          // Arithmetic operators saturate towards positive or negative infinity.
                          // Infinity values short circuit expressions.

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
#define POS_MAX FLT_MAX
#define POS_MIN FLT_MIN

typedef double length_t; // Length

double otfcc_from_f2dot14(const f2dot14 x);
int16_t otfcc_to_f2dot14(const double x);
double otfcc_from_fixed(const f16dot16 x);
f16dot16 otfcc_to_fixed(const double x);

#define f16dot16_precision 16
#define f16dot16_k (1 << (f16dot16_precision - 1))
#define f16dot16_infinity ((f16dot16)0x7fffffff)
#define f16dot16_negativeIntinity ((f16dot16)0x80000000)

f16dot16 otfcc_f1616_add(f16dot16 a, f16dot16 b);
f16dot16 otfcc_f1616_minus(f16dot16 a, f16dot16 b);
f16dot16 otfcc_f1616_multiply(f16dot16 a, f16dot16 b);
f16dot16 otfcc_f1616_divide(f16dot16 a, f16dot16 b);
f16dot16 otfcc_f1616_muldiv(f16dot16 a, f16dot16 b, f16dot16 c);

#endif
