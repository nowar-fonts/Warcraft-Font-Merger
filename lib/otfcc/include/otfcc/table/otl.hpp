#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <iterator>
#include <memory>
#include <set>
#include <string>
#include <utility>
#include <variant>
#include <vector>

#include <nlohmann/json.hpp>

#include "table-common.h"

namespace otfcc::table {

struct otl {

	struct coverage : std::vector<glyph_handle> {
		using base = std::vector<glyph_handle>;

		coverage(buffer_view buf) {
			static char info[4096];
			if (buf.length() < 4)
				throw exception::too_short{
				    .name = "otl::coverage",
				    .info = "reading format and glyphCount",
				    .expected = 4,
				    .actual = buf.length(),
				};
			uint16_t format = buf.read16u();
			switch (format) {
			case 1: {
				uint16_t glyphCount = buf.read16u();
				if (buf.length() < 4 + glyphCount * 2) {
					snprintf(info, sizeof info, "glyphCount = %d", glyphCount);
					throw exception::too_short{
					    .name = "otl::coverage",
					    .info = info,
					    .expected = size_t(4 + glyphCount * 2),
					    .actual = buf.length(),
					};
				}
				std::set<glyphid_t> occupied;
				for (uint16_t j = 0; j < glyphCount; j++) {
					glyphid_t gid = buf.read16u();
					if (occupied.find(gid) == occupied.end()) {
						occupied.insert(gid);
						push_back(handle{gid});
					}
				}
				break;
			}
			case 2: {
				uint16_t rangeCount = buf.read16u();
				if (buf.length() < 4 + rangeCount * 6) {
					snprintf(info, sizeof info, "rangeCount = %d", rangeCount);
					throw exception::too_short{
					    .name = "otl::coverage",
					    .info = info,
					    .expected = size_t(4 + rangeCount * 6),
					    .actual = buf.length(),
					};
				}
				std::set<glyphid_t> occupied;
				for (uint16_t j = 0; j < rangeCount; j++) {
					glyphid_t start = buf.read16u();
					glyphid_t end = buf.read16u();
					glyphid_t startCoverageIndex = buf.read16u();
					for (int32_t k = start; k <= end; k++) {
						glyphid_t gid = k;
						if (occupied.find(gid) == occupied.end()) {
							occupied.insert(gid);
							push_back(handle{gid});
						}
					}
				}
				break;
			}
			default:
				throw exception::unknown_format{
				    .name = "otl::coverage",
				    .format = format,
				};
			}
		}

		coverage(const nlohmann::json &json) {
			if (!json.is_array())
				return;
			for (const nlohmann::json &name : json) {
				if (name.is_string())
					push_back(handle{std::string(name)});
			}
		}

		void sort_unique() {
			std::sort(begin(), end(),
			          [](const handle &a, const handle &b) { return a.index < b.index; });
			auto it = std::unique(begin(), end(), [](const handle &a, const handle &b) {
				return a.index == b.index;
			});
			resize(it - begin());
		}

		void consolidate(const glyph_order &gord) {
			std::vector<glyph_handle> consolidated;
			for (glyph_handle &h : *this)
				if (gord.consolidate_handle(h))
					consolidated.push_back(h);
			swap(consolidated);
			sort_unique();
		}

		void shrink(bool dosort) {
			std::vector<glyph_handle> shrunk;
			for (glyph_handle &h : *this)
				if (!h.name.empty())
					shrunk.push_back(h);
			swap(shrunk);
			if (dosort)
				sort_unique();
		}

		// assume consolidated
		nlohmann::json to_json() const {
			nlohmann::json result = nlohmann::json::array();
			std::transform(begin(), end(), std::back_inserter(result),
			               [](const glyph_handle &h) { return h.name; });
			return result;
		}

		buffer build_format1() const {
			buffer result;
			result.append16(1);
			result.append16(size());
			for (const glyph_handle &h : *this)
				result.append16(h.index);
			return result;
		}

		buffer build_format2() const {
			buffer result;
			result.append16(2);
			buffer ranges;
			glyphid_t startGid = begin()->index;
			glyphid_t endGid = startGid;
			glyphid_t lastGid = startGid;
			size_t nRanges = 0;
			for (size_t j = 1; j < size(); j++) {
				glyphid_t current = (*this)[j].index;
				if (current <= lastGid)
					continue;
				if (current == endGid + 1)
					endGid = current;
				else {
					ranges.append16(startGid);
					ranges.append16(endGid);
					ranges.append16(j + startGid - endGid - 1);
					nRanges += 1;
					startGid = endGid = current;
				}
				lastGid = current;
			}
			ranges.append16(startGid);
			ranges.append16(endGid);
			ranges.append16(size() + startGid - endGid - 1);
			nRanges += 1;
			result.append16(nRanges);
			result.append(ranges);
			return result;
		}

		// assume consolidated
		buffer build(uint16_t format = 0) const {
			switch (format) {
			case 1:
				return build_format1();
			case 2:
				return build_format2();
			default:
				auto format1 = build_format1();
				auto format2 = build_format2();
				if (format1.length() <= format2.length())
					return format1;
				else
					return format2;
			}
		}
	};

	struct class_definition : std::vector<std::pair<glyph_handle, glyphclass_t>> {
		glyphclass_t maxclass;

		using base = std::vector<std::pair<glyph_handle, glyphclass_t>>;

		class_definition(buffer_view buf) {
			static char info[4096];
			if (buf.length() < 4)
				throw exception::too_short{
				    .name = "otl::class_definition",
				    .info = "reading format and count",
				    .expected = 4,
				    .actual = buf.length(),
				};
			uint16_t format = buf.read16u();
			switch (format) {
			case 1: {
				if (buf.length() < 6)
					throw exception::too_short{
					    .name = "otl::class_definition",
					    .info = "reading format 1 startGlyphID and glyphCount",
					    .expected = 6,
					    .actual = buf.length(),
					};
				uint16_t startGid = buf.read16u();
				uint16_t count = buf.read16u();
				if (buf.length() < 6 + count * 2) {
					snprintf(info, sizeof info, "glyphCount = %d", count);
					throw exception::too_short{
					    .name = "otl::class_definition",
					    .info = info,
					    .expected = size_t(6 + count * 2),
					    .actual = buf.length(),
					};
				}
				for (uint16_t j = 0; j < count; j++) {
					glyphclass_t klass = buf.read16u();
					emplace_back(handle{glyphid_t(startGid + j)}, klass);
				}
				break;
			}
			case 2: {
				uint16_t rangeCount = buf.read16u();
				if (buf.length() < 4 + rangeCount * 6) {
					snprintf(info, sizeof info, "rangeCount = %d", rangeCount);
					throw exception::too_short{
					    .name = "otl::class_definition",
					    .info = info,
					    .expected = size_t(4 + rangeCount * 6),
					    .actual = buf.length(),
					};
				}
				// the ranges may overlap.
				std::set<glyphid_t> occupied;
				for (uint16_t j = 0; j < rangeCount; j++) {
					glyphid_t start = buf.read16u();
					glyphid_t end = buf.read16u();
					glyphid_t klass = buf.read16u();
					for (int32_t k = start; k <= end; k++) {
						glyphid_t gid = k;
						if (occupied.find(gid) == occupied.end()) {
							occupied.insert(gid);
							emplace_back(handle{gid}, klass);
						}
					}
				}
				break;
			}
			default:
				throw exception::unknown_format{
				    .name = "otl::coverage",
				    .format = format,
				};
			}
		}

		class_definition(const nlohmann::json &json) {
			if (!json.is_object())
				return;
			for (const auto &[name, klass] : json.items()) {
				if (klass.is_number())
					emplace_back(handle{name}, glyphclass_t(klass));
			}
		}

		void extend(const coverage &cov) {
			std::set<glyphid_t> occupied;
			std::transform(begin(), end(), std::inserter(occupied, occupied.end()),
			               [](const auto &item) { return item.first.index; });
			for (const handle &h : cov)
				if (occupied.find(h.index) == occupied.end()) {
					occupied.insert(h.index);
					emplace_back(h, 0);
				}
		}

		void consolidate(const glyph_order &gord) {
			std::vector<std::pair<glyph_handle, glyphclass_t>> consolidated;
			for (auto &[handle, klass] : *this) {
				if (gord.consolidate_handle(handle))
					consolidated.emplace_back(handle, klass);
			}
			std::sort(consolidated.begin(), consolidated.end(),
			          [](const auto &a, const auto &b) { return a.first.index < b.first.index; });
			auto it = std::unique(
			    consolidated.begin(), consolidated.end(),
			    [](const auto &a, const auto &b) { return a.first.index == b.first.index; });
			consolidated.resize(it - consolidated.begin());
			swap(consolidated);
		}

		// assume consolidated
		nlohmann::json to_json() const {
			nlohmann::json result;
			std::transform(begin(), end(), std::inserter(result, result.end()),
			               [](const auto &h) { return h.first.name; });
			return result;
		}

		// assume consolidated
		buffer build() const {
			buffer result;
			result.append16(2);
			if (!size()) {
				result.append16(0);
				return result;
			}

			struct record {
				glyphid_t gid;
				glyphclass_t klass;
			};
			std::vector<record> nonTrivial;
			for (const auto &[handle, klass] : *this)
				if (klass)
					nonTrivial.emplace_back(handle.index, klass);
			if (!nonTrivial.size()) {
				// the class def has only class 0
				result.append16(0);
				return result;
			}
			std::sort(nonTrivial.begin(), nonTrivial.end(),
			          [](record a, record b) { return a.gid < b.gid; });

			glyphid_t startGid = nonTrivial[0].gid;
			glyphid_t endGid = startGid;
			glyphclass_t lastClass = nonTrivial[0].klass;
			size_t nRanges = 0;
			glyphid_t lastGid = startGid;
			buffer ranges;
			for (size_t j = 1; j < nonTrivial.size(); j++) {
				glyphid_t current = nonTrivial[j].gid;
				if (current <= lastGid)
					continue;
				if (current == endGid + 1 && nonTrivial[j].klass == lastClass)
					endGid = current;
				else {
					ranges.append16(startGid);
					ranges.append16(endGid);
					ranges.append16(lastClass);
					nRanges += 1;
					startGid = endGid = current;
					lastClass = nonTrivial[j].klass;
				}
				lastGid = current;
			}
			ranges.append16(startGid);
			ranges.append16(endGid);
			ranges.append16(lastClass);
			nRanges += 1;
			result.append16(nRanges);
			result.append(ranges);
		}
	};

	enum class lookup_type {
		unknown = 0,

		gsub_unknown = 0x10,
		gsub_single = 0x11,
		gsub_multiple = 0x12,
		gsub_alternate = 0x13,
		gsub_ligature = 0x14,
		gsub_context = 0x15,
		gsub_chaining = 0x16,
		gsub_extend = 0x17,
		gsub_reverse = 0x18,

		gpos_unknown = 0x20,
		gpos_single = 0x21,
		gpos_pair = 0x22,
		gpos_cursive = 0x23,
		gpos_markToBase = 0x24,
		gpos_markToLigature = 0x25,
		gpos_markToMark = 0x26,
		gpos_context = 0x27,
		gpos_chaining = 0x28,
		gpos_extend = 0x29,
	};

	typedef union _otl_subtable otl_Subtable;
	typedef struct _otl_lookup otl_Lookup;

	struct position_value {
		pos_t dx;
		pos_t dy;
		pos_t dWidth;
		pos_t dHeight;
	};

	struct gsub_single_entry {
		glyph_handle from;
		glyph_handle to;
	};

	using gsub_single = std::vector<gsub_single_entry>;

	struct gsub_multi_entry {
		glyph_handle from;
		std::vector<glyph_handle> to;
	};

	using gsub_multi = std::vector<gsub_multi_entry>;

	struct gsub_ligature_entry {
		std::vector<glyph_handle> from;
		glyph_handle to;
	};

	using gsub_ligature = std::vector<gsub_ligature_entry>;

	enum class chaining_type {
		canonical =
		    0,    // The canonical form of chaining contextual substitution, one rule per subtable.
		poly = 1, // The multi-rule form, right after reading OTF. N rule per subtable.
		classified = 2, // The classified intermediate form, for building TTF with compression.
		                // N rules, has classdefs, and coverage GID interpreted as class number.
	};

	struct clain_lookup_application {
		tableid_t index;
		lookup_handle lookup;
	};

	struct chaining_rule {
		tableid_t inputBegins;
		tableid_t inputEnds;
		std::vector<coverage> match;
		std::vector<clain_lookup_application> apply;
	};

	struct subtable_chaining {

		struct poly_chain {
			std::vector<std::vector<chaining_rule>> rules;
			std::vector<class_definition> bc;
			std::vector<class_definition> ic;
			std::vector<class_definition> fc;
		};

		chaining_type type;
		std::variant<chaining_rule, poly_chain> rule;
	};

	struct gsub_reverse {
		tableid_t inputIndex;
		std::vector<std::vector<coverage>> match;
		std::vector<coverage> to;
	};

	struct gpos_single_entry {
		glyph_handle target;
		position_value value;
	};

	using gpos_single = std::vector<gpos_single_entry>;

	struct anchor {
		bool present;
		pos_t x;
		pos_t y;
	};

	struct gpos_pair {
		std::vector<class_definition> first;
		std::vector<class_definition> second;
		std::vector<std::vector<position_value>> firstValues;
		std::vector<std::vector<position_value>> secondValues;
	};

	struct gpos_cursive_entry {
		glyph_handle target;
		anchor enter;
		anchor exit;
	};

	using gpos_cursive = std::vector<gpos_cursive_entry>;

	struct mark_record {
		glyph_handle glyph;
		glyphclass_t markClass;
		anchor anchor;
	};

	using mark_array = std::vector<mark_record>;

	struct base_record {
		glyph_handle glyph;
		std::vector<anchor> anchors;
	};

	using base_array = std::vector<base_record>;

	struct gpos_mark_to_single {
		mark_array markArray;
		base_array baseArray;
	};

	struct ligature_base_record {
		glyph_handle glyph;
		std::vector<std::vector<anchor>> anchors;
	};

	using ligature_array = std::vector<ligature_base_record>;

	struct gpos_mark_to_ligature {
		mark_array markArray;
		ligature_array ligArray;
	};

	struct subtable;

	struct extend {
		lookup_type type;
		std::vector<std::shared_ptr<subtable>> subtable;
	};

	struct subtable : std::variant<gsub_single, gsub_multi, gsub_ligature, subtable_chaining,
	                               gsub_reverse, gpos_single, gpos_pair, gpos_cursive,
	                               gpos_mark_to_single, gpos_mark_to_ligature, extend> {};

	using subtable_list = std::vector<std::shared_ptr<subtable>>;

	struct lookup {
		std::string name;
		lookup_type type;
		uint32_t _offset;
		uint16_t flags;
		subtable_list subtables;
	};

	using lookup_list = std::vector<std::shared_ptr<lookup>>;
	using lookup_ref_list = std::vector<std::weak_ptr<lookup>>;

	struct feature {
		std::string name;
		lookup_ref_list lookups;
	};

	using feature_list = std::vector<std::shared_ptr<feature>>;
	using feature_ref = std::weak_ptr<feature>;
	using feature_ref_list = std::vector<std::weak_ptr<feature>>;

	struct language {
		std::string name;
		feature_ref requiredFeature;
		feature_list features;
	};

	using language_list = std::vector<std::shared_ptr<language>>;

	lookup_list lookups;
	feature_list features;
	language_list languages;
};

} // namespace otfcc::table
