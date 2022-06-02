#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace otfcc::table {

struct fpgm_prep {
	std::string tag;
	std::vector<uint8_t> bytes;
};

} // namespace otfcc::table
