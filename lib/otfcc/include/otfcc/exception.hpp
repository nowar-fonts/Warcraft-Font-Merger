#pragma once

#include <stdexcept>

namespace otfcc::exception {

struct glyph_format_error : std::runtime_error {
	explicit glyph_format_error(const std::string &s) : runtime_error(s) {}
	explicit glyph_format_error(const char *s) : runtime_error(s) {}

	glyph_format_error(const glyph_format_error &) noexcept = default;
	virtual ~glyph_format_error() noexcept = default;
};

} // namespace otfcc::exception
