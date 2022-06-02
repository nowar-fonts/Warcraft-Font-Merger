#include "extend.h"

// Extended tables are special
// We will only deal with reading, and they will be flatten.
static otl_Subtable *_caryll_read_otl_extend(font_file_pointer data, uint32_t tableLength,
                                             uint32_t subtableOffset, otl_LookupType BASIS,
                                             const glyphid_t maxGlyphs,
                                             const otfcc_Options *options) {
	otl_Subtable *_subtable;
	NEW(_subtable);
	checkLength(subtableOffset + 8);
	subtable_extend *subtable = &(_subtable->extend);
	subtable->type = read_16u(data + subtableOffset + 2) + BASIS;
	subtable->subtable = otfcc_readOtl_subtable(
	    data, tableLength, subtableOffset + read_32u(data + subtableOffset + 4), subtable->type,
	    maxGlyphs, options);
	goto OK;
FAIL:
	FREE(_subtable);
OK:
	return _subtable;
}

otl_Subtable *otfcc_readOtl_gsub_extend(font_file_pointer data, uint32_t tableLength,
                                        uint32_t subtableOffset, const glyphid_t maxGlyphs,
                                        const otfcc_Options *options) {
	return _caryll_read_otl_extend(data, tableLength, subtableOffset, otl_type_gsub_unknown,
	                               maxGlyphs, options);
}
otl_Subtable *otfcc_readOtl_gpos_extend(font_file_pointer data, uint32_t tableLength,
                                        uint32_t subtableOffset, const glyphid_t maxGlyphs,
                                        const otfcc_Options *options) {
	return _caryll_read_otl_extend(data, tableLength, subtableOffset, otl_type_gpos_unknown,
	                               maxGlyphs, options);
}
