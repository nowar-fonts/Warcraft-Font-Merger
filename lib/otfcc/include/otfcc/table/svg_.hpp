#pragma once

#include <vector>

#include "../buffer.hpp"
#include "../primitives.hpp"

namespace otfcc::table::svg {

struct assignment {
	// Due to the outward references from SVG document to GID.
	// We have to use GID here. Sorry.
	glyphid_t start;
	glyphid_t end;
	std::vector<buffer> document;
};

using table = std::vector<assignment>;

}
