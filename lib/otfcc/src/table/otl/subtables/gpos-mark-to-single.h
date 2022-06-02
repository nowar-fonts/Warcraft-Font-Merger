#ifndef CARYLL_TABLE_OTL_GPOS_MARK_TO_SINGLE_H
#define CARYLL_TABLE_OTL_GPOS_MARK_TO_SINGLE_H

#include "common.h"

otl_Subtable *otl_read_gpos_markToSingle(const font_file_pointer data, uint32_t tableLength,
                                         uint32_t subtableOffset, const glyphid_t maxGlyphs,
                                         const otfcc_Options *options);
json_value *otl_gpos_dump_markToSingle(const otl_Subtable *st);
otl_Subtable *otl_gpos_parse_markToSingle(const json_value *_subtable,
                                          const otfcc_Options *options);
caryll_Buffer *otfcc_build_gpos_markToSingle(const otl_Subtable *_subtable, otl_BuildHeuristics heuristics);

#endif
