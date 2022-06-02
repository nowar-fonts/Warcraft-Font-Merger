#pragma once

#include <cstdint>
#include <map>
#include <string>

#include "../handle.hpp"

namespace otfcc::table {

// We will support format 0, 4, 12 and 14 of CMAP only
struct cmap {

	struct uvs_key {
		uint32_t unicode;
		uint32_t selector;
	};

	std::map<int, glyph_handle> unicodes;
	std::map<uvs_key, glyph_handle> uvs;

	bool encode_by_index(int c, uint16_t gid) {
		if (unicodes.find(c) == unicodes.end()) {
			unicodes[c] = handle{gid};
			return true;
		} else
			return false;
	}

	bool encode_by_name(int c, std::string name) {
		if (unicodes.find(c) == unicodes.end()) {
			unicodes[c] = handle{name};
			return true;
		} else
			return false;
	}

	bool unmap(int c) {
		if (unicodes.find(c) != unicodes.end()) {
			unicodes.erase(c);
			return true;
		} else
			return false;
	}

	handle &operator[](int c) { return unicodes[c]; }

	bool encode_by_index(uvs_key c, uint16_t gid) {
		if (uvs.find(c) == uvs.end()) {
			uvs[c] = handle{gid};
			return true;
		} else
			return false;
	}

	bool encode_by_name(uvs_key c, std::string name) {
		if (uvs.find(c) == uvs.end()) {
			uvs[c] = handle{name};
			return true;
		} else
			return false;
	}

	bool unmap(uvs_key c) {
		if (uvs.find(c) != uvs.end()) {
			uvs.erase(c);
			return true;
		} else
			return false;
	}

	handle &operator[](uvs_key c) { return uvs[c]; }
};

} // namespace otfcc::table
