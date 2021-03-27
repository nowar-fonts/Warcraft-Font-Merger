#ifndef cff_DATA_TYPES
#define cff_DATA_TYPES

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "support/util.h"

#include "cff-util.h"
#include "cff-value.h"
#include "cff-index.h"
#include "cff-dict.h"
#include "cff-charset.h"
#include "cff-fdselect.h"

// Limits of Type2 CharSrting
enum {
	type2_argument_stack = 48,
	type2_stem_hints = 96,
	type2_subr_nesting = 10,
	type2_charstring_len = 65535,
	type2_max_subrs = 65300,
	type2_transient_array = 32,
};

typedef struct {
	uint8_t major;
	uint8_t minor;
	uint8_t hdrSize;
	uint8_t offSize;
} cff_Header;

// CFF Encoding Structures
typedef struct {
	uint8_t format;
	uint8_t ncodes;
	uint8_t *code;
} cff_EncodingFormat0;

typedef struct {
	uint8_t first;
	uint8_t nleft;
} cff_EncodingRangeFormat1;

typedef struct {
	uint8_t format;
	uint8_t nranges;
	cff_EncodingRangeFormat1 *range1;
} cff_EncodingFormat1;

typedef struct {
	uint8_t code;
	uint16_t glyph;
} cff_EncodingSupplement;

typedef struct {
	uint8_t nsup;
	cff_EncodingSupplement *supplement;
} cff_EncodingNS;

typedef struct {
	uint32_t t;
	union {
		cff_EncodingFormat0 f0;
		cff_EncodingFormat1 f1;
		cff_EncodingNS ns;
	};
} cff_Encoding;

// Predefined Encoding Types
enum {
	cff_FONT_COMMON,
	cff_FONT_CID,
	cff_FONT_MM,
};

enum {
	cff_ENC_STANDARD,
	cff_ENC_EXPERT,
	cff_ENC_FORMAT0,
	cff_ENC_FORMAT1,
	cff_ENC_FORMAT_SUPPLEMENT,
	cff_ENC_UNSPECED,
};

typedef struct {
	cff_Value *stack;
	cff_Value transient[32];
	arity_t index;
	arity_t max;
	uint8_t stem;
} cff_Stack;

typedef struct {
	uint8_t *raw_data;
	uint32_t raw_length;
	uint16_t cnt_glyph;

	cff_Header head;
	cff_Index name;
	cff_Index top_dict;
	cff_Index string;
	cff_Index global_subr;

	cff_Encoding encodings; // offset
	cff_Charset charsets;   // offset
	cff_FDSelect fdselect;  // offset
	cff_Index char_strings; // offset
	cff_Index font_dict;    // offset
	cff_Index local_subr;   // offset
} cff_File;

// Outline builder method table
typedef struct {
	void (*setWidth)(void *context, double width);
	void (*newContour)(void *context);
	void (*lineTo)(void *context, double x1, double y1);
	void (*curveTo)(void *context, double x1, double y1, double x2, double y2, double x3, double y3);
	void (*setHint)(void *context, bool isVertical, double position, double width);
	void (*setMask)(void *context, bool isContourMask, bool *mask);
	double (*getrand)(void *context);
} cff_IOutlineBuilder;

/*
  CFF -> Compact Font Format
  CS2 -> Type2 CharString
*/

extern char *op_cff_name(uint32_t op);
extern char *op_cs2_name(uint32_t op);
uint8_t cff_getStandardArity(uint32_t op);

sds sdsget_cff_sid(uint16_t idx, cff_Index str);

extern uint32_t cff_decodeCffToken(const uint8_t *start, cff_Value *val);
extern uint32_t cff_decodeCS2Token(const uint8_t *start, cff_Value *val);

// number, number, float
extern caryll_Buffer *cff_encodeCffOperator(int32_t val);
extern caryll_Buffer *cff_encodeCffInteger(int32_t val);
extern caryll_Buffer *cff_encodeCffFloat(double val);

/*
  Writer
*/

extern caryll_Buffer *cff_buildOffset(int32_t val);
extern caryll_Buffer *cff_buildHeader(void);

void cff_mergeCS2Int(caryll_Buffer *blob, int32_t val);
void cff_mergeCS2Operator(caryll_Buffer *blob, int32_t val);
void cff_mergeCS2Operand(caryll_Buffer *blob, double val);
void cff_mergeCS2Special(caryll_Buffer *blob, uint8_t val);

extern uint8_t cff_parseSubr(uint16_t idx, uint8_t *raw, cff_Index fdarray, cff_FDSelect select, cff_Index *subr);
void cff_parseOutline(uint8_t *data, uint32_t len, cff_Index gsubr, cff_Index lsubr, cff_Stack *stack, void *outline,
                      cff_IOutlineBuilder methods, const otfcc_Options *options);

// File
extern cff_File *cff_openStream(uint8_t *data, uint32_t len, const otfcc_Options *options);
extern void cff_close(cff_File *file);

#endif
