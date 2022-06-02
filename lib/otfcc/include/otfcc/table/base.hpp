#pragma once

#include <vector>

#include "otl.hpp"
#include "table-common.h"

namespace otfcc::table {

struct base {

	struct value {
		uint32_t tag;
		pos_t coordinate;
	};
	struct script_entry {
		uint32_t tag;
		uint32_t defaultBaselineTag;
		tableid_t baseValuesCount;
		std::vector<value> baseValues;
	};

	struct axis {
		tableid_t scriptCount;
		std::vector<script_entry> entries;
	};

	std::vector<axis> horizontal;
	std::vector<axis> vertical;
};

} // namespace otfcc::table
