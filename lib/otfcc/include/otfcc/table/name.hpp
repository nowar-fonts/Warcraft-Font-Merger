#pragma once

#include <cstdint>
#include <string>
#include <vector>

namespace otfcc::table::name {

struct record {
	uint16_t platformID;
	uint16_t encodingID;
	uint16_t languageID;
	uint16_t nameID;
	std::string nameString;
};

using table = std::vector<record>;

}
