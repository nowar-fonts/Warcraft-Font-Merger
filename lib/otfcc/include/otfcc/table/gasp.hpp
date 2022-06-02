#pragma once

#include <vector>

#include "../primitives.hpp"

namespace otfcc::table {

struct gasp {

	struct record {
		glyphsize_t rangeMaxPPEM;
		bool dogray;
		bool gridfit;
		bool symmetric_smoothing;
		bool symmetric_gridfit;
	};

	uint16_t version;
	std::vector<record> records;
};

} // namespace otfcc::table
