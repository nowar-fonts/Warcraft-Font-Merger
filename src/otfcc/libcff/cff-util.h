#ifndef CARYLL_cff_UTIL_H
#define CARYLL_cff_UTIL_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "support/util.h"

// clang-format off
// CFF DICT Operators
enum {
	op_version          = 0x00, op_Copyright          = 0x0c00,
	op_Notice           = 0x01, op_isFixedPitch       = 0x0c01,
	op_FullName         = 0x02, op_ItalicAngle        = 0x0c02,
	op_FamilyName       = 0x03, op_UnderlinePosition  = 0x0c03,
	op_Weight           = 0x04, op_UnderlineThickness = 0x0c04,
	op_FontBBox         = 0x05, op_PaintType          = 0x0c05,
	op_BlueValues       = 0x06, op_CharstringType     = 0x0c06,
	op_OtherBlues       = 0x07, op_FontMatrix         = 0x0c07,
	op_FamilyBlues      = 0x08, op_StrokeWidth        = 0x0c08,
	op_FamilyOtherBlues = 0x09, op_BlueScale          = 0x0c09,
	op_StdHW            = 0x0a, op_BlueShift          = 0x0c0a,
	op_StdVW            = 0x0b, op_BlueFuzz           = 0x0c0b,
	/* 0x0c escape */           op_StemSnapH          = 0x0c0c,
	op_UniqueID         = 0x0d, op_StemSnapV          = 0x0c0d,
	op_XUID             = 0x0e, op_ForceBold          = 0x0c0e,
	op_charset          = 0x0f, /* 0x0c0f Reserved */
	op_Encoding         = 0x10, /* 0x0c10 Reserved */
	op_CharStrings      = 0x11, op_LanguageGroup      = 0x0c11,
	op_Private          = 0x12, op_ExpansionFactor    = 0x0c12,
	op_Subrs            = 0x13, op_initialRandomSeed  = 0x0c13,
	op_defaultWidthX    = 0x14, op_SyntheicBase       = 0x0c14,
	op_nominalWidthX    = 0x15, op_PostScript         = 0x0c15,
	op_vsindex          = 0x16, op_BaseFontName       = 0x0c16,
	op_blend            = 0x17, op_BaseFontBlend      = 0x0c17,
	op_vstore           = 0x18, /* 0x0c18 Reserved */
	op_maxstack         = 0x19, /* 0x0c19 Reserved */
								/* 0x0c1a Reserved */
								/* 0x0c1b Reserved */
								/* 0x0c1c Reserved */
								/* 0x0c1d Reserved */
								op_ROS                = 0x0c1e,
								op_CIDFontVersion     = 0x0c1f,
								op_CIDFontRevision    = 0x0c20,
								op_CIDFontType        = 0x0c21,
								op_CIDCount           = 0x0c22,
								op_UIDBase            = 0x0c23,
								op_FDArray            = 0x0c24,
								op_FDSelect           = 0x0c25,
								op_FontName           = 0x0c26,
};

// Type2 CharString Operators
enum {
	/* 0x00 Reserved */   /* 0x0c00 Reserved */
	op_hstem      = 0x01, /* 0x0c01 Reserved */
	/* 0x02 Reserved */   /* 0x0c02 Reserved */
	op_vstem      = 0x03, op_and    = 0x0c03,
	op_vmoveto    = 0x04, op_or     = 0x0c04,
	op_rlineto    = 0x05, op_not    = 0x0c05,
	op_hlineto    = 0x06, /* 0x0c06 Reserved */
	op_vlineto    = 0x07, /* 0x0c07 Reserved */
	op_rrcurveto  = 0x08, /* 0x0c08 Reserved */
	/* 0x09 Reserved */   op_abs    = 0x0c09,
	op_callsubr   = 0x0a, op_add    = 0x0c0a,
	op_return     = 0x0b, op_sub    = 0x0c0b,
	/* 0x0c escape   */   op_div    = 0x0c0c,
	/* 0x0d Reserved */   /* 0x0c0d Reserved */
	op_endchar    = 0x0e, op_neg    = 0x0c0e,
	op_cff2vsidx  = 0x0f, op_eq     = 0x0c0f,
	op_cff2blend  = 0x10, /* 0x0c10 Reserved */
	/* 0x11 Reserved */   /* 0x0c11 Reserved */
	op_hstemhm    = 0x12, op_drop   = 0x0c12,
	op_hintmask   = 0x13, /* 0x0c13 Reserved */
	op_cntrmask   = 0x14, op_put    = 0x0c14,
	op_rmoveto    = 0x15, op_get    = 0x0c15,
	op_hmoveto    = 0x16, op_ifelse = 0x0c16,
	op_vstemhm    = 0x17, op_random = 0x0c17,
	op_rcurveline = 0x18, op_mul    = 0x0c18,
	op_rlinecurve = 0x19, /* 0x0c19 Reserved */
	op_vvcurveto  = 0x1a, op_sqrt   = 0x0c1a,
	op_hhcurveto  = 0x1b, op_dup    = 0x0c1b,
	/* 0x1c short int */  op_exch   = 0x0c1c,
	op_callgsubr  = 0x1d, op_index  = 0x0c1d,
	op_vhcurveto  = 0x1e, op_roll   = 0x0c1e,
	op_hvcurveto  = 0x1f, /* 0x0c1f Reserved */
						/* 0x0c20 Reserved */
						/* 0x0c21 Reserved */
						op_hflex  = 0x0c22,
						op_flex   = 0x0c23,
						op_hflex1 = 0x0c24,
						op_flex1  = 0x0c25,
};
// clang-format on

// parser util functions
static INLINE uint32_t gu1(uint8_t *s, uint32_t p) {
	uint32_t b0 = *(s + p);
	return b0;
}

static INLINE uint32_t gu2(uint8_t *s, uint32_t p) {
	uint32_t b0 = *(s + p) << 8;
	uint32_t b1 = *(s + p + 1);
	return b0 | b1;
}

static INLINE uint32_t gu3(uint8_t *s, uint32_t p) {
	uint32_t b0 = *(s + p) << 16;
	uint32_t b1 = *(s + p + 1) << 8;
	uint32_t b2 = *(s + p + 2);
	return b0 | b1 | b2;
}
static INLINE uint32_t gu4(uint8_t *s, uint32_t p) {
	uint32_t b0 = *(s + p) << 24;
	uint32_t b1 = *(s + p + 1) << 16;
	uint32_t b2 = *(s + p + 2) << 8;
	uint32_t b3 = *(s + p + 3);
	return b0 | b1 | b2 | b3;
}

#endif
