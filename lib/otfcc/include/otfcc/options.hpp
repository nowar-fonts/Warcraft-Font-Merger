#pragma once

#include <bits/stdint-uintn.h>
#include <memory>
#include <stdint.h>
#include <stdlib.h>
#include <string>

#include "logger.hpp"

namespace otfcc {

struct options {
	bool debug_wait_on_start;
	bool ignore_glyph_order;
	bool ignore_hints;
	bool has_vertical_metrics;
	bool export_fdselect;
	bool keep_average_char_width;
	bool keep_unicode_ranges;
	bool short_post;
	bool dummy_DSIG;
	bool keep_modified_time;
	bool instr_as_bytes;
	bool verbose;
	bool quiet;
	bool cff_short_vmtx;
	bool merge_lookups;
	bool merge_features;
	bool force_cid;
	bool cff_rollCharString;
	bool cff_doSubroutinize;
	bool stub_cmap4;
	bool decimal_cmap;
	bool name_glyphs_by_hash;
	bool name_glyphs_by_gid;
	std::string glyph_name_prefix;
	std::unique_ptr<logger::base> logger;

	options() = default;
	~options() = default;

	void optimise_to(uint8_t level) {
		cff_rollCharString = false;
		short_post = false;
		ignore_glyph_order = false;
		cff_short_vmtx = false;
		merge_features = false;
		force_cid = false;
		cff_doSubroutinize = false;

		if (level >= 1) {
			cff_rollCharString = true;
			cff_short_vmtx = true;
		}
		if (level >= 2) {
			short_post = true;
			cff_doSubroutinize = true;
			merge_features = true;
		}
		if (level >= 3) {
			ignore_glyph_order = true;
			force_cid = true;
		}
	}
};

} // namespace otfcc
