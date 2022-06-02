#pragma once

#include <cstdint>
#include <string>

#include "primitives.hpp"

namespace otfcc {

struct handle {

	enum class state_t {
		empty = 0,
		indexed = 1,
		name = 2,
		consolidated = 3,
	};

	state_t state;
	glyphid_t index;
	std::string name;

	handle() : state(), index(), name() {}
	handle(glyphid_t gid) : handle(gid, std::string(), state_t::indexed) {}
	handle(std::string name)
	    : state(name.length() ? state_t::name : state_t::empty), index(0), name(name) {}
	// from consolidated
	handle(glyphid_t gid, std::string name)
	    : state(state_t::consolidated), index(gid), name(name) {}
	handle(glyphid_t gid, std::string name, state_t state) : state(state), index(gid), name(name) {}

	void reset() { *this = handle{}; }
};

struct glyph_handle : handle {
	glyph_handle(const handle &handle_) : handle(handle_) {}
};

struct fd_handle : handle {
	fd_handle(const handle &handle_) : handle(handle_) {}
};

struct lookup_handle : handle {
	lookup_handle(const handle &handle_) : handle(handle_) {}
};

struct axis_handle : handle {
	axis_handle(const handle &handle_) : handle(handle_) {}
};

} // namespace otfcc
