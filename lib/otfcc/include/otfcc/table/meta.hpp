#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace otfcc::table {

struct meta {

	struct entry {
		uint32_t tag;
		std::string data;
	};

	uint32_t version;
	uint32_t flags;
	std::vector<entry> entries;
};

} // namespace otfcc::table
