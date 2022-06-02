#pragma once

#include <array>
#include <climits>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <string>

#include <nlohmann/json.hpp>

#include "otfcc/primitives.h"

#if defined(OTFCC_ENABLE_VARIATION) && OTFCC_ENABLE_VARIATION

#include "otfcc/table/fvar.h"
#include "otfcc/vf/vq.h"

#endif

namespace otfcc::json {

inline nlohmann::json preserialize(const nlohmann::json &x);

inline const nlohmann::json &json_object_push_tag(nlohmann::json &object, uint32_t tag,
                                                  const nlohmann::json &value) {
	std::string tags{char(tag >> 24), char(tag >> 16), char(tag >> 8), char(tag)};
	object[tags] = value;
	return value;
}

inline nlohmann::json json_new_position(pos_t z) {
	if (round(z) == z)
		return int64_t(z);
	else
		return z;
}

#if defined(OTFCC_ENABLE_VARIATION) && OTFCC_ENABLE_VARIATION

nlohmann::json json_new_VQRegion_Explicit(const vq_Region *rs, const table_fvar *fvar);
nlohmann::json json_new_VQRegion(const vq_Region *rs, const table_fvar *fvar);
nlohmann::json json_new_VQ(const VQ z, const table_fvar *fvar);
nlohmann::json json_new_VV(const VV x, const table_fvar *fvar);
nlohmann::json json_new_VVp(const VV *x, const table_fvar *fvar);
VQ json_vqOf(const nlohmann::json cv, const table_fvar *fvar);

#endif

// flags reader and writer
template <size_t N> inline nlohmann::json dump_flags(uint32_t flags, const char *(&labels)[N]) {
	static_assert(N < sizeof flags * CHAR_BIT);
	nlohmann::json v{};
	for (size_t j = 0; j < N; j++)
		if (flags & (1 << j))
			v[labels[j]] = true;
	return v;
}
template <size_t N> inline uint32_t parse_flags(const nlohmann::json &v, const char *(&labels)[N]) {
	static_assert(N < sizeof parse_flags(v, labels) * CHAR_BIT);
	if (v.is_object()) {
		uint32_t flags = 0;
		for (size_t j = 0; j < N; j++)
			if (v.contains(labels[j]) && v[labels[j]])
				flags |= (1 << j);
		return flags;
	} else
		return 0;
}

} // namespace otfcc::json
