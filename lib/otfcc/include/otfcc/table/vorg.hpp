#pragma once

#include <cstdint>
#include <vector>

#include "../primitives.hpp"

namespace otfcc::table {

struct vorg {

	struct entry {
		glyphid_t gid;
		int16_t verticalOrigin;
	};

	glyphid_t numVertOriginYMetrics;
	pos_t defaultVerticalOrigin;
	std::vector<entry> entries;
};

} // namespace otfcc::table
