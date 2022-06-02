#pragma once

#include "maxp.hpp"
#include "table-common.h"

namespace otfcc::table {

struct hdmx {

	struct device_record {
		uint8_t pixelSize;
		uint8_t maxWidth;
		std::vector<uint8_t> widths;
	};

	uint16_t version;
	uint16_t numRecords;
	uint32_t sizeDeviceRecord;
	std::vector<device_record> records;
};

} // namespace otfcc::table
