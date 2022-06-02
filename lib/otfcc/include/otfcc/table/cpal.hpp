#pragma once

#include <cstdint>
#include <vector>

namespace otfcc::table {

struct cpal {

	struct color {
		uint8_t red;
		uint8_t green;
		uint8_t blue;
		uint8_t alpha;
		uint16_t label;
	};

	struct palette {
		std::vector<color> colorset;
		uint32_t type;
		uint32_t label;
	};

	uint16_t version;
	std::vector<palette> palettes;
};

} // namespace otfcc::table
