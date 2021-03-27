#ifndef CARYLL_INCLUDE_OPTIONS_H
#define CARYLL_INCLUDE_OPTIONS_H

#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include "logger.h"

typedef struct {
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
	char *glyph_name_prefix;
	otfcc_ILogger *logger;
} otfcc_Options;

otfcc_Options *otfcc_newOptions();
void otfcc_deleteOptions(otfcc_Options *options);
void otfcc_Options_optimizeTo(otfcc_Options *options, uint8_t level);

#endif
