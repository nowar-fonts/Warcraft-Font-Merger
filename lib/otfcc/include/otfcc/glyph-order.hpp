#pragma once

#include <cstdio>
#include <string>
#include <unordered_map>

#include "handle.hpp"
#include "primitives.hpp"

namespace otfcc {

class glyph_order {
  public:
	struct entry {
		glyphid_t gid;
		std::string name;
		uint8_t orderType;
		uint32_t orderEntry;
	};

  private:
	std::unordered_map<glyphid_t, entry> _by_gid;
	std::unordered_map<std::string, entry> _by_name;

  public:
	// register a gid->name map
	std::string set_by_gid(glyphid_t gid, std::string name) {
		static char u8buffer[4096];
		if (_by_gid.find(gid) != _by_gid.end()) {
			// gid is already in the order table.
			// reject this naming suggestion.
			return _by_gid[gid].name;
		} else {
			if (_by_name.find(name) != _by_name.end()) {
				// The name is already in-use.
				snprintf(u8buffer, sizeof u8buffer, "$$gid%d", gid);
				name = u8buffer;
			}
			entry e{gid, name};
			_by_gid[gid] = e;
			_by_name[name] = e;
			return name;
		}
	}

	// register a name->gid map
	bool set_by_name(std::string name, glyphid_t gid) {
		if (_by_name.find(name) != _by_name.end()) {
			// name is already mapped to a glyph
			// reject this naming suggestion
			return false;
		} else {
			entry e{gid, name};
			_by_gid[gid] = e;
			_by_name[name] = e;
			return true;
		}
	}

	bool name_a_field(glyphid_t gid, std::string &field) {
		if (_by_gid.find(gid) != _by_gid.end()) {
			field = _by_gid[gid].name;
			return true;
		} else {
			field = "";
			return false;
		}
	}

	bool consolidate_handle(handle &h) const {
		switch (h.state) {
		case handle::state_t::consolidated: {
			auto it_name = _by_name.find(h.name);
			if (it_name != _by_name.end()) {
				h = handle{it_name->second.gid, it_name->second.name};
				return true;
			}
			auto it_gid = _by_gid.find(h.index);
			if (it_gid != _by_gid.end()) {
				h = handle{it_gid->second.gid, it_gid->second.name};
				return true;
			}
			return false;
		}
		case handle::state_t::name: {
			auto it_name = _by_name.find(h.name);
			if (it_name != _by_name.end()) {
				h = handle{it_name->second.gid, it_name->second.name};
				return true;
			} else {
				return false;
			}
		}
		case handle::state_t::indexed: {
			auto it_gid = _by_gid.find(h.index);
			if (it_gid != _by_gid.end()) {
				h = handle{it_gid->second.gid, it_gid->second.name};
				return true;
			} else {
				return false;
			}
		}
		default:
			return false;
		}
	}

	bool lookup_name(std::string name) { return _by_name.find(name) != _by_name.end(); }
};

} // namespace otfcc
