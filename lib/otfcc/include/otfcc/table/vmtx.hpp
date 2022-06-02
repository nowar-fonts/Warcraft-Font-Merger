#pragma once

#include <vector>

#include "../primitives.hpp"

namespace otfcc::table {

struct vmtx {
	struct vertical_metric {
		length_t advanceHeight;
		pos_t tsb;
	};

	std::vector<vertical_metric> metrics;
	std::vector<pos_t> topSideBearing;
};

} // namespace otfcc::table
