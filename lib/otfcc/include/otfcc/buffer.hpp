#pragma once

#include "type.hpp"
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <iterator>
#include <string>
#include <string_view>
#include <vector>

namespace otfcc {

struct Buffer : std::string {

	struct ReservedOffset {
		size_t pOffset;
		std::string data;
	};

	std::vector<ReservedOffset> _pending16;
	std::vector<ReservedOffset> _pending32;
	size_t _current;

	Buffer() = default;
	Buffer(const std::string &v) : std::string(v) {}
	Buffer(std::string &&v) : std::string(std::move(v)) {}
	Buffer(const Buffer &) = default;
	Buffer(Buffer &&) = default;
	~Buffer() = default;

	Buffer &operator=(const Buffer &) = default;
	Buffer &operator=(Buffer &&) = default;

	void finish();
};

struct BufferView : std::string_view {

	size_t _current;

	BufferView() = default;
	BufferView(const BufferView &) = default;
	BufferView(const std::string_view &v) : std::string_view(v) {}
	BufferView(const std::string &v) : std::string_view(v) {}

	BufferView &operator++() {
		_current++;
		return *this;
	}
	BufferView &operator+=(size_t n) {
		_current += n;
		return *this;
	}

	BufferView &operator>>(uint8 &o) {
		o = (*this)[_current];
		_current++;
		return *this;
	}
	BufferView &operator>>(int8 &o) {
		o = (*this)[_current];
		_current++;
		return *this;
	}
	BufferView &operator>>(uint16 &o) {
		uint8_t b0, b1;
		(*this) >> b0 >> b1;
		o = b0 << 8 | b1;
		return *this;
	}
	BufferView &operator>>(int16 &o) {
		uint8_t b0, b1;
		(*this) >> b0 >> b1;
		o = b0 << 8 | b1;
		return *this;
	}
	BufferView &operator>>(uint24 &o) {
		uint8_t b0, b1, b2;
		(*this) >> b0 >> b1 >> b2;
		o = b0 << 16 | b1 << 8 | b2;
		return *this;
	}
	BufferView &operator>>(uint32 &o) {
		uint8_t b0, b1, b2, b3;
		(*this) >> b0 >> b1 >> b2 >> b3;
		o = b0 << 24 | b1 << 16 | b2 << 8 | b3;
		return *this;
	}
	BufferView &operator>>(int32 &o) {
		uint8_t b0, b1, b2, b3;
		(*this) >> b0 >> b1 >> b2 >> b3;
		o = b0 << 24 | b1 << 16 | b2 << 8 | b3;
		return *this;
	}
	BufferView &operator>>(Fixed &o) { return *this >> o._value; }
	BufferView &operator>>(F2DOT14 &o) { return *this >> o._value; }
	BufferView &operator>>(LONGDATETIME &o) {
		uint8_t b0, b1, b2, b3, b4, b5, b6, b7;
		(*this) >> b0 >> b1 >> b2 >> b3 >> b4 >> b5 >> b6 >> b7;
		o = int64_t(b0) << 56 | int64_t(b1) << 48 | int64_t(b2) << 40 | int64_t(b3) << 32 |
		    b4 << 24 | b5 << 16 | b6 << 8 | b7;
		return *this;
	}
	BufferView &operator>>(Tag &o) {
		o._value[0] = (*this)[_current];
		o._value[1] = (*this)[_current + 1];
		o._value[2] = (*this)[_current + 2];
		o._value[3] = (*this)[_current + 3];
		_current += 4;
		return *this;
	}
	BufferView &operator>>(Version16Dot16 &o) { return *this >> o._value; }

	std::string read_offset16() {
		Offset16 offset;
		uint16 length;
		(*this) >> offset >> length;
		return {begin() + offset, begin() + offset + length};
	}
	std::string readOffset32() {
		Offset32 offset;
		uint32 length;
		(*this) >> offset >> length;
		return {begin() + offset, begin() + offset + length};
	}
};

} // namespace otfcc
