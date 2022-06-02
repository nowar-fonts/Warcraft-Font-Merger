#pragma once

#include <vector>

#include "../handle.hpp"
#include "../primitives.hpp"

namespace otfcc::table::colr {

struct layer {
	glyph_handle glyph;
	colorid_t paletteIndex;
};

struct mapping {
	glyph_handle glyph;
	std::vector<layer> layers;
};

using table = std::vector<mapping>;

} // namespace otfcc::table::colr
