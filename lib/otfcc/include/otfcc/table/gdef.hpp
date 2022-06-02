#pragma once

#include <vector>

#include "otfcc/handle.hpp"
#include "table-common.h"
#include "otl.hpp"

namespace otfcc::table {

struct caret_value {
	int8_t format;
	pos_t coordinate;
	int16_t pointIndex;
};

struct caret_value_record {
	glyph_handle glyph;
	std::vector<caret_value> carets;
};

struct gdef {
	std::vector<otl::class_definition> glyphClassDef;
	std::vector<otl::class_definition> markAttachClassDef;
	std::vector<caret_value_record> ligCarets;
};

}
