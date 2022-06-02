#pragma once

#include <cstdint>
#include <vector>

namespace otfcc::table {

struct vdmx {

	struct record {
		uint16_t yPelHeight;
		int16_t yMax;
		int16_t yMin;
	};

	struct ratio_range {
		uint8_t bCharset;
		uint8_t xRatio;
		uint8_t yStartRatio;
		uint8_t yEndRatio;
		std::vector<record> records;
	};

	uint16_t version;
	std::vector<ratio_range> ratios;
};

} // namespace otfcc::table
