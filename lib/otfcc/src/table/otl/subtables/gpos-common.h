#ifndef CARYLL_TABLE_OTL_GPOS_COMMON_H
#define CARYLL_TABLE_OTL_GPOS_COMMON_H

#include "common.h"

// anchors
otl_Anchor otl_anchor_absent();
otl_Anchor otl_read_anchor(font_file_pointer data, uint32_t tableLength, uint32_t offset);
json_value *otl_dump_anchor(otl_Anchor a);
otl_Anchor otl_parse_anchor(json_value *v);
bk_Block *bkFromAnchor(otl_Anchor a);

// mark arrays
typedef struct {
	sds className;
	glyphclass_t classID;
	UT_hash_handle hh;
} otl_ClassnameHash;

void otl_readMarkArray(otl_MarkArray *array, otl_Coverage *cov, font_file_pointer data,
                       uint32_t tableLength, uint32_t offset);
void otl_parseMarkArray(json_value *_marks, otl_MarkArray *array, otl_ClassnameHash **h,
                        const otfcc_Options *options);

// position values
extern const uint8_t FORMAT_DX;
extern const uint8_t FORMAT_DY;
extern const uint8_t FORMAT_DWIDTH;
extern const uint8_t FORMAT_DHEIGHT;
extern const uint8_t bits_in[0x100];

uint8_t position_format_length(uint16_t format);
otl_PositionValue position_zero();
otl_PositionValue read_gpos_value(font_file_pointer data, uint32_t tableLength, uint32_t offset,
                                  uint16_t format);
uint8_t required_position_format(otl_PositionValue v);
void write_gpos_value(caryll_Buffer *buf, otl_PositionValue v, uint16_t format);
bk_Block *bk_gpos_value(otl_PositionValue v, uint16_t format);

json_value *gpos_dump_value(otl_PositionValue value);
otl_PositionValue gpos_parse_value(json_value *pos);

#endif
