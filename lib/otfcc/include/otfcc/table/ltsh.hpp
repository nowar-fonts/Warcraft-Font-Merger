#pragma once

#include <cstdint>
#include <vector>

#include "../primitives.hpp"

namespace otfcc::table {

struct ltsh {
	uint16_t version;
	glyphid_t numGlyphs;
	std::vector<uint8_t> yPels;
};

}
