#pragma once

#include <vector>

#include "../glyph-order.hpp"
#include "../primitives.hpp"

namespace otfcc::table {

struct post {
	fixed version;
	fixed italicAngle;
	int16_t underlinePosition;
	int16_t underlineThickness;
	uint32_t isFixedPitch;
	uint32_t minMemType42;
	uint32_t maxMemType42;
	uint32_t minMemType1;
	uint32_t maxMemType1;
	std::vector<glyph_order> post_name_map;
};

}
