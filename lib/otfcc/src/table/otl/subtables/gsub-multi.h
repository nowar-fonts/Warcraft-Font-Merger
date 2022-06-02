#ifndef CARYLL_TABLE_OTL_GSUB_MULTI_H
#define CARYLL_TABLE_OTL_GSUB_MULTI_H

#include "common.h"

otl_Subtable *otl_read_gsub_multi(const font_file_pointer data, uint32_t tableLength,
                                  uint32_t subtableOffset, const glyphid_t maxGlyphs,
                                  const otfcc_Options *options);
json_value *otl_gsub_dump_multi(const otl_Subtable *_subtable);
otl_Subtable *otl_gsub_parse_multi(const json_value *_subtable, const otfcc_Options *options);
caryll_Buffer *otfcc_build_gsub_multi_subtable(const otl_Subtable *_subtable, otl_BuildHeuristics heuristics);

#endif
