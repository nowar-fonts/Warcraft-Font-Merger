#pragma once

#include <compare>
#include <memory>
#include <string>
#include <vector>

#include "../handle.hpp"
#include "../primitives.hpp"
#include "head.hpp"
#include "maxp.hpp"
#include "table-common.h"

#if defined(OTFCC_ENABLE_VARIATION) && OTFCC_ENABLE_VARIATION
#include "fvar.h"
#endif

namespace otfcc::table::glyf {

enum type { simple, composite };

struct point {
#if defined(OTFCC_ENABLE_VARIATION) && OTFCC_ENABLE_VARIATION
	VQ x;
	VQ y;
#else
	pos_t x;
	pos_t y;
#endif
	int8_t on; // a mask indicates whether a point is on-curve or off-curve
	           // bit 0     : 1 for on-curve, 0 for off-curve. JSON field: "on"
	           // bit 1 - 7 : unused, set to 0
	           // in JSON, they are separated into several boolean fields.
};

using contour = std::vector<point>;
using contour_list = std::vector<contour>;

// CFF stems and hint masks
struct cff_stem {
	pos_t position;
	pos_t width;
	uint16_t map;

	std::partial_ordering operator<=>(const cff_stem &rhs) {
		return std::pair{position, map} <=> std::pair{rhs.position, rhs.map};
	}
};
using stem_list = std::vector<cff_stem>;

struct cff_hint_mask {
	uint16_t pointsBefore;
	uint16_t contoursBefore;
	bool maskH[0x100];
	bool maskV[0x100];

	auto operator<=>(const cff_hint_mask &rhs) {
		return std::pair{contoursBefore, pointsBefore} <=>
		       std::pair{rhs.contoursBefore, rhs.pointsBefore};
	}
};
using mask_list = std::vector<cff_hint_mask>;

typedef enum {
	REF_XY = 0,
	REF_ANCHOR_ANCHOR = 1,
	REF_ANCHOR_XY = 2,
	REF_ANCHOR_CONSOLIDATED = 3,
	REF_ANCHOR_CONSOLIDATING_ANCHOR = 4,
	REF_ANCHOR_CONSOLIDATING_XY = 5,
} RefAnchorStatus;

struct component {
	//// NOTE: this part and below looks like a glyf_Point
#if defined(OTFCC_ENABLE_VARIATION) && OTFCC_ENABLE_VARIATION
	VQ x;
	VQ y;
#else
	pos_t x;
	pos_t y;
#endif
	// flags
	bool roundToGrid;
	bool useMyMetrics;
	// the glyph being referenced
	otfcc::glyph_handle glyph;
	// transformation term
	scale_t a;
	scale_t b;
	scale_t c;
	scale_t d;
	// Anchorness term
	RefAnchorStatus isAnchored;
	shapeid_t inner;
	shapeid_t outer;
};

using reference_list = std::vector<component>;

struct stat {
	pos_t xMin;
	pos_t xMax;
	pos_t yMin;
	pos_t yMax;
	uint16_t nestDepth;
	uint16_t nPoints;
	uint16_t nContours;
	uint16_t nCompositePoints;
	uint16_t nCompositeContours;
};

struct glyph {
	std::string name;

	// Metrics
#if defined(OTFCC_ENABLE_VARIATION) && OTFCC_ENABLE_VARIATION
	VQ horizontalOrigin;
	VQ advanceWidth;
	VQ verticalOrigin;
	VQ advanceHeight;
#else
	pos_t horizontalOrigin;
	pos_t advanceWidth;
	pos_t verticalOrigin;
	pos_t advanceHeight;
#endif

	// Outline
	// NOTE: SFNT does not support mixed glyphs, but we do.
	contour_list contours;
	reference_list references;

	// Postscript hints
	stem_list stemH;
	stem_list stemV;
	mask_list hintMasks;
	mask_list contourMasks;

	// TTF instructions
	std::string instructions;
	// TTF Screen specific
	uint8_t yPel;

	// CID FDSelect
	fd_handle fdSelect;
	glyphid_t cid; // Subset CID fonts may need this to represent the original CID entry

	// Stats
	stat stat;
};

using glyph_ptr = std::shared_ptr<glyph>;
using table = std::vector<glyph_ptr>;

} // namespace otfcc::table::glyf
