#pragma once

#include <cstddef>

#include "../buffer.hpp"
#include "../glyph-order.hpp"
#include "../handle.hpp"
#include "../options.hpp"
#include "../primitives.hpp"
#include "../sfnt.h"

#if defined(OTFCC_ENABLE_VARIATION) && OTFCC_ENABLE_VARIATION
#include "otfcc/vf/vq.h"
#endif

namespace otfcc::table {

namespace exception {

struct too_short {
	const char *name;
	const char *info;
	size_t expected;
	size_t actual;
};

struct unknown_format {
	const char *name;
	int32_t format;
};

} // namespace exception

} // namespace otfcc::table
