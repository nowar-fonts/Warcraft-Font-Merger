#pragma once

#include <memory>
#include <string>
#include <vector>

#include "../primitives.hpp"
#include "glyf.hpp"
#include "head.hpp"
#include "table-common.h"

namespace otfcc::table {

struct cff_ {

	struct font_matrix {
		scale_t a;
		scale_t b;
		scale_t c;
		scale_t d;
#if defined(OTFCC_ENABLE_VARIATION) && OTFCC_ENABLE_VARIATION
		VQ x;
		VQ y;
#else
		pos_t x;
		pos_t y;
#endif
	};

	struct private_dict {
		std::vector<double> blueValues;
		std::vector<double> otherBlues;
		std::vector<double> familyBlues;
		std::vector<double> familyOtherBlues;
		double blueScale;
		double blueShift;
		double blueFuzz;
		double stdHW;
		double stdVW;
		std::vector<double> stemSnapH;
		std::vector<double> stemSnapV;
		bool forceBold;
		uint32_t languageGroup;
		double expansionFactor;
		double initialRandomSeed;
		double defaultWidthX;
		double nominalWidthX;
	};

	// Name
	std::string fontName;

	// General properties
	bool isCID;
	std::string version;
	std::string notice;
	std::string copyright;
	std::string fullName;
	std::string familyName;
	std::string weight;
	bool isFixedPitch;
	double italicAngle;
	double underlinePosition;
	double underlineThickness;
	double fontBBoxTop;
	double fontBBoxBottom;
	double fontBBoxLeft;
	double fontBBoxRight;
	double strokeWidth;
	std::vector<private_dict> privateDict;
	std::vector<font_matrix> fontMatrix;

	// CID-only operators
	std::string cidRegistry;
	std::string cidOrdering;
	uint32_t cidSupplement;
	double cidFontVersion;
	double cidFontRevision;
	uint32_t cidCount;
	uint32_t UIDBase;
	// CID FDArray
	std::vector<std::unique_ptr<cff_>> fdArray;
};

struct cff_glyf {
	cff_ meta;
	glyf::table glyphs;
};

} // namespace otfcc::table
