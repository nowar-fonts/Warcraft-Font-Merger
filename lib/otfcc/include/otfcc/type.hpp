#pragma once

#include <cmath>
#include <compare>
#include <concepts>
#include <cstddef>
#include <cstdint>
#include <ostream>
#include <string>

namespace otfcc {

using uint8 = std::uint8_t;
using int8 = std::int8_t;
using uint16 = std::uint16_t;
using int16 = std::int16_t;

struct uint24 {
	std::uint32_t _value;

	constexpr uint24() = default;
	constexpr uint24(const uint24 &) = default;
	constexpr uint24(uint32_t v) : _value(v & 0xffffff) {}

	constexpr uint24 &operator=(const uint24 &) = default;
	constexpr uint24 &operator=(uint32_t v) {
		_value = v & 0xffffff;
		return *this;
	}

	constexpr operator uint32_t() const { return _value & 0xffffff; }

	constexpr std::strong_ordering operator<=>(const uint24 &) const = default;
};

using uint32 = std::uint32_t;
using int32 = std::int32_t;

struct Fixed {
	std::uint32_t _value;

	constexpr Fixed() = default;
	constexpr Fixed(const Fixed &) = default;
	constexpr Fixed(std::floating_point auto v) : _value(std::round(v * 0x10000)) {}

	constexpr Fixed &operator=(const Fixed &) = default;
	constexpr Fixed &operator=(std::floating_point auto v) {
		_value = v * 0x10000;
		return *this;
	}

	template <std::floating_point T> constexpr operator T() const { return _value / T(0x10000); }

	constexpr std::strong_ordering operator<=>(const Fixed &) const = default;
};

using FWORD = std::int16_t;
using UFWORD = std::uint16_t;

struct F2DOT14 {
	std::int16_t _value;

	constexpr F2DOT14() = default;
	constexpr F2DOT14(const F2DOT14 &) = default;
	constexpr F2DOT14(std::floating_point auto v) : _value(std::round(v * 0x4000)) {}

	constexpr F2DOT14 &operator=(const F2DOT14 &) = default;
	constexpr F2DOT14 &operator=(std::floating_point auto v) {
		_value = v * 0x4000;
		return *this;
	}

	template <std::floating_point T> constexpr operator T() const { return _value / T(0x4000); }

	constexpr std::strong_ordering operator<=>(const F2DOT14 &) const = default;
};

struct LONGDATETIME {
	std::int64_t _value;

	constexpr LONGDATETIME() = default;
	constexpr LONGDATETIME(const LONGDATETIME &) = default;
	constexpr LONGDATETIME(std::int64_t v) : _value(v) {}

	constexpr LONGDATETIME &operator=(const LONGDATETIME &) = default;

	static constexpr std::int64_t epochOffset = -2082844800;
	static constexpr LONGDATETIME fromUnixEpoch(std::int64_t v) { return {v - epochOffset}; }
	constexpr std::int64_t toUnixEpoch() const { return _value + epochOffset; }
};

struct Tag {
	uint8 _value[4];

	constexpr Tag() : _value{' ', ' ', ' ', ' '} {};
	constexpr Tag(const Tag &) = default;
	constexpr Tag(const char (&)[1]) : Tag() {}
	constexpr Tag(const char (&v)[2]) : _value{uint8(v[0]), ' ', ' ', ' '} {}
	constexpr Tag(const char (&v)[3]) : _value{uint8(v[0]), uint8(v[1]), ' ', ' '} {}
	constexpr Tag(const char (&v)[4])
	    : _value{uint8(v[0]), uint8(v[1]), uint8(v[2]), ' '} {}
	constexpr Tag(const char (&v)[5])
	    : _value{uint8(v[0]), uint8(v[1]), uint8(v[2]), uint8(v[3])} {}

	constexpr Tag &operator=(const Tag &) = default;
	template <size_t N>
	constexpr Tag &operator=(const char (&v)[N])
	    requires(N >= 1 && N <= 5)
	{
		return operator=(Tag{v});
	};

	operator std::string() const { return {_value, _value + 4}; }

	constexpr std::strong_ordering operator<=>(const Tag &) const = default;
};

inline std::ostream &operator<<(std::ostream &s, const Tag &t) { return s << std::string(t); }

using Offset16 = std::uint16_t;
using Offset24 = uint24;
using Offset32 = std::uint32_t;

struct Version16Dot16 {
	uint32 _value;

	static constexpr uint32 convertValue(std::floating_point auto v) {
		uint32 fixed = std::round(v * 10000);
		return ((fixed / 10000) << 16) | ((fixed % 10000 / 1000) << 12) |
		       ((fixed % 1000 / 100) << 8) | ((fixed % 100 / 10) << 4) | (fixed % 10);
	}

	constexpr Version16Dot16() = default;
	constexpr Version16Dot16(const Version16Dot16 &) = default;
	constexpr Version16Dot16(std::floating_point auto v) : _value(convertValue(v)) {}

	template <std::floating_point T> constexpr operator T() const {
		uint32 fixed = ((_value & 0xffff0000) >> 16) * 10000 + ((_value & 0xf000) >> 12) * 1000 +
		                 ((_value & 0xf00) >> 8) * 100 + ((_value & 0xf0) >> 4) * 10 +
		                 (_value & 0xf);
		return fixed / T(10000);
	}
};

} // namespace otfcc
