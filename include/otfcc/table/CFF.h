#ifndef CARYLL_INCLUDE_TABLE_CFF_H
#define CARYLL_INCLUDE_TABLE_CFF_H

#include "table-common.h"
#include "head.h"
#include "glyf.h"

typedef struct {
	scale_t a;
	scale_t b;
	scale_t c;
	scale_t d;
	VQ x;
	VQ y;
} cff_FontMatrix;

typedef struct {
	arity_t blueValuesCount;
	OWNING double *blueValues;
	arity_t otherBluesCount;
	OWNING double *otherBlues;
	arity_t familyBluesCount;
	OWNING double *familyBlues;
	arity_t familyOtherBluesCount;
	OWNING double *familyOtherBlues;
	double blueScale;
	double blueShift;
	double blueFuzz;
	double stdHW;
	double stdVW;
	arity_t stemSnapHCount;
	OWNING double *stemSnapH;
	arity_t stemSnapVCount;
	OWNING double *stemSnapV;
	bool forceBold;
	uint32_t languageGroup;
	double expansionFactor;
	double initialRandomSeed;
	double defaultWidthX;
	double nominalWidthX;
} cff_PrivateDict;

typedef struct _table_CFF table_CFF;

struct _table_CFF {
	// Name
	sds fontName;

	// General properties
	bool isCID;
	sds version;
	sds notice;
	sds copyright;
	sds fullName;
	sds familyName;
	sds weight;
	bool isFixedPitch;
	double italicAngle;
	double underlinePosition;
	double underlineThickness;
	double fontBBoxTop;
	double fontBBoxBottom;
	double fontBBoxLeft;
	double fontBBoxRight;
	double strokeWidth;
	OWNING cff_PrivateDict *privateDict;
	OWNING cff_FontMatrix *fontMatrix;

	// CID-only operators
	sds cidRegistry;
	sds cidOrdering;
	uint32_t cidSupplement;
	double cidFontVersion;
	double cidFontRevision;
	uint32_t cidCount;
	uint32_t UIDBase;
	// CID FDArray
	tableid_t fdArrayCount;
	OWNING table_CFF **fdArray;
};

extern caryll_RefElementInterface(table_CFF) table_iCFF;

// CFF and glyf
typedef struct {
	OWNING table_CFF *meta;
	OWNING table_glyf *glyphs;
} table_CFFAndGlyf;

#endif
