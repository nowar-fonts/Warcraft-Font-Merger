#ifndef CARYLL_TABLE_OTL_CHAINING_H
#define CARYLL_TABLE_OTL_CHAINING_H

#include "common.h"

// This file supports four formats
// GSUB contextual
// GSUB chaining
// GPOS contextual
// GPOS chaining

otl_Subtable *otl_read_chaining(const font_file_pointer data, uint32_t tableLength, uint32_t offset,
                                const glyphid_t maxGlyphs, const otfcc_Options *options);
otl_Subtable *otl_read_contextual(const font_file_pointer data, uint32_t tableLength,
                                  uint32_t offset, const glyphid_t maxGlyphs,
                                  const otfcc_Options *options);
json_value *otl_dump_chaining(const otl_Subtable *_subtable);
otl_Subtable *otl_parse_chaining(const json_value *_subtable, const otfcc_Options *options);
caryll_Buffer *otfcc_build_chaining(const otl_Subtable *_subtable);
caryll_Buffer *otfcc_build_contextual(const otl_Subtable *_subtable);

tableid_t otfcc_classifiedBuildChaining(const otl_Lookup *lookup,
                                        OUT caryll_Buffer ***subtableBuffers,
                                        MODIFY size_t *lastOffset);

bool otfcc_chainingLookupIsContextualLookup(const otl_Lookup *lookup);

#endif
