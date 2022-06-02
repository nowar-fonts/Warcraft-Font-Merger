#pragma once

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <memory>
#include <numeric>
#include <stdexcept>
#include <string>
#include <vector>

#include "buffer.hpp"
#include "exception.hpp"
#include "type.hpp"

namespace otfcc::sfnt {

struct TableDirectory {

	static constexpr uint32 versionTtf = 0x00010000;
	static constexpr uint32 versionOtto = 0x4F54544F;

	static uint32 checksum(const std::string &raw_data) {
		// pad to four-byte aligned offset with zero
		const std::size_t expected_length = (raw_data.length() + 3) & ~3;
		const std::size_t num_uint32s = expected_length / 4;
		std::vector<uint32> buffer(num_uint32s);
		std::memcpy(buffer.data(), raw_data.data(), raw_data.length());

		return std::accumulate(buffer.cbegin(), buffer.cend(), uint32(0));
	}

	std::map<Tag, std::string> tableRecords;

	constexpr uint32 sfntVersion() const {
		bool glyf = tableRecords.contains("glyf");
		bool cff = tableRecords.contains("CFF ");
		bool cff2 = tableRecords.contains("CFF2");

		if (glyf + cff + cff2 != 1)
			throw exception::glyph_format_error(
			    "OpenType font should contain exactly 1 of `glyf`, `CFF ` or "
			    "`CFF2` table.");
		return glyf ? versionTtf : versionOtto;
	}

	constexpr uint16 numTables() const { return tableRecords.size(); }
	constexpr uint16 searchRange() const { return (1 << entrySelector()) * 16; }
	constexpr uint16 entrySelector() const { return std::floor(std::log2(numTables())); }
	constexpr uint16 rangeShift() const { return numTables() * 16 - searchRange(); }
};

struct ttc_header {
	uint16 major_version;
	uint16 minor_version;
	std::vector<TableDirectory> table_directories;

	static constexpr uint32 tag_null = 0;
	static constexpr uint32 tag_dsig = 0x44534947;

	constexpr uint32 num_fonts() const { return table_directories.size(); }
	constexpr uint32 dsig_tag() const { return tag_null; }
	constexpr uint32 dsig_length() const { return 0; }
	constexpr uint32 dsig_offset() const { return 0; }
};

inline bool isTtc(const std::string &raw_file) {
	return raw_file.length() > 16 && raw_file[0] == 't' && raw_file[1] == 't' &&
	       raw_file[2] == 'c' && raw_file[3] == 'f';
}

inline bool isTtf(const std::string &raw_file) {
	return raw_file.length() > 12 && raw_file[0] == 0 && raw_file[1] == 1 && raw_file[2] == 0 &&
	       raw_file[3] == 0;
}

inline bool isOtf(const std::string &raw_file) {
	return raw_file.length() > 12 && raw_file[0] == 'O' && raw_file[1] == 'T' &&
	       raw_file[2] == 'T' && raw_file[3] == 'O';
}

inline bool isOpentype(const std::string &raw_file) {
	return isTtc(raw_file) || isTtf(raw_file) || isOtf(raw_file);
}

inline bool isSingleSfnt(const std::string &raw_file) { return isTtf(raw_file) && isOtf(raw_file); }

inline TableDirectory readSfnt(const std::string &raw_file) {
	BufferView bv(raw_file);

	uint16 num_tables;
	bv += 4; // sfnt version
	bv >> num_tables;
	bv += 6; // search range, entry selector, range shift

	TableDirectory td;
	for (uint16 i = 0; i < num_tables; i++) {
		Tag tag;
		bv >> tag;
		bv += 4; // checksum
		td.tableRecords[tag] = bv.readOffset32();
	}
	return td;
}

} // namespace otfcc::sfnt
