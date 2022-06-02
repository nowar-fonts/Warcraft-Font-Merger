#ifndef CARYLL_TABLE_OTL_GPOS_PAIR_H
#define CARYLL_TABLE_OTL_GPOS_PAIR_H

#include "common.h"

otl_Subtable *otl_read_gpos_pair(const font_file_pointer data, uint32_t tableLength,
                                 uint32_t offset, const glyphid_t maxGlyphs,
                                 const otfcc_Options *options);
json_value *otl_gpos_dump_pair(const otl_Subtable *_subtable);
otl_Subtable *otl_gpos_parse_pair(const json_value *_subtable, const otfcc_Options *options);
caryll_Buffer *otfcc_build_gpos_pair(const otl_Subtable *_subtable, otl_BuildHeuristics heuristics);

#endif
