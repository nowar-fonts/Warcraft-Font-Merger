#pragma once

#include <vector>

#include "hhea.hpp"
#include "maxp.hpp"
#include "../primitives.hpp"

namespace otfcc::table {

struct hmtx {

	struct horizontal_metric {
		length_t advanceWidth;
		pos_t lsb;
	};

	std::vector<horizontal_metric> metrics;
	std::vector<pos_t> leftSideBearing;
};

} // namespace otfcc::table
