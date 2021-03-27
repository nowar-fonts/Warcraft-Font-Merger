#include "gpos-common.h"

static void deleteMarkArrayItem(otl_MarkRecord *entry) {
	Handle.dispose(&entry->glyph);
}
static caryll_ElementInterface(otl_MarkRecord) gss_typeinfo = {
    .init = NULL, .copy = NULL, .dispose = deleteMarkArrayItem};

caryll_standardVectorImpl(otl_MarkArray, otl_MarkRecord, gss_typeinfo, otl_iMarkArray);

void otl_readMarkArray(otl_MarkArray *array, otl_Coverage *cov, font_file_pointer data,
                       uint32_t tableLength, uint32_t offset) {
	checkLength(offset + 2);
	glyphid_t markCount = read_16u(data + offset);
	for (glyphid_t j = 0; j < markCount; j++) {
		glyphclass_t markClass = read_16u(data + offset + 2 + j * 4);
		uint16_t delta = read_16u(data + offset + 2 + j * 4 + 2);
		if (delta) {
			otl_iMarkArray.push(
			    array,
			    ((otl_MarkRecord){.glyph = Handle.dup(cov->glyphs[j]),
			                      .markClass = markClass,
			                      .anchor = otl_read_anchor(data, tableLength, offset + delta)}));
		} else {
			otl_iMarkArray.push(array, ((otl_MarkRecord){.glyph = Handle.dup(cov->glyphs[j]),
			                                             .markClass = markClass,
			                                             .anchor = otl_anchor_absent()}));
		}
	}
FAIL:
	return;
}

static int compare_classHash(otl_ClassnameHash *a, otl_ClassnameHash *b) {
	return strcmp(a->className, b->className);
}
void otl_parseMarkArray(json_value *_marks, otl_MarkArray *array, otl_ClassnameHash **h,
                        const otfcc_Options *options) {
	for (glyphid_t j = 0; j < _marks->u.object.length; j++) {
		otl_MarkRecord mark;
		char *gname = _marks->u.object.values[j].name;
		json_value *anchorRecord = _marks->u.object.values[j].value;
		mark.glyph = Handle.fromName(sdsnewlen(gname, _marks->u.object.values[j].name_length));
		mark.markClass = 0;
		mark.anchor = otl_anchor_absent();

		if (!anchorRecord || anchorRecord->type != json_object) {
			otl_iMarkArray.push(array, mark);
			continue;
		}
		json_value *_className = json_obj_get_type(anchorRecord, "class", json_string);
		if (!_className) {
			otl_iMarkArray.push(array, mark);
			continue;
		}

		sds className = sdsnewlen(_className->u.string.ptr, _className->u.string.length);
		otl_ClassnameHash *s;
		HASH_FIND_STR(*h, className, s);
		if (!s) {
			NEW(s);
			s->className = className;
			s->classID = HASH_COUNT(*h);
			HASH_ADD_STR(*h, className, s);
		} else {
			sdsfree(className);
		}
		mark.markClass = s->classID;
		mark.anchor.present = true;
		mark.anchor.x = json_obj_getnum(anchorRecord, "x");
		mark.anchor.y = json_obj_getnum(anchorRecord, "y");
		otl_iMarkArray.push(array, mark);
	}

	HASH_SORT(*h, compare_classHash);
	glyphid_t jAnchorIndex = 0;
	otl_ClassnameHash *s;
	foreach_hash(s, *h) {
		s->classID = jAnchorIndex;
		jAnchorIndex++;
	}
	for (glyphid_t j = 0; j < array->length; j++) {
		if (!array->items[j].anchor.present) continue;
		json_value *anchorRecord = _marks->u.object.values[j].value;
		json_value *_className = json_obj_get_type(anchorRecord, "class", json_string);
		sds className = sdsnewlen(_className->u.string.ptr, _className->u.string.length);
		otl_ClassnameHash *s;
		HASH_FIND_STR(*h, className, s);
		if (s) {
			array->items[j].markClass = s->classID;
		} else {
			array->items[j].markClass = 0;
		}
		sdsfree(className);
	}
}

otl_Anchor otl_anchor_absent() {
	otl_Anchor anchor = {.present = false, .x = 0, .y = 0};
	return anchor;
}
otl_Anchor otl_read_anchor(font_file_pointer data, uint32_t tableLength, uint32_t offset) {
	otl_Anchor anchor = {.present = false, .x = 0, .y = 0};
	checkLength(offset + 6);
	anchor.present = true;
	anchor.x = read_16s(data + offset + 2);
	anchor.y = read_16s(data + offset + 4);
	return anchor;
FAIL:
	anchor.present = false;
	anchor.x = 0;
	anchor.y = 0;
	return anchor;
}
json_value *otl_dump_anchor(otl_Anchor a) {
	if (a.present) {
		json_value *v = json_object_new(2);
		json_object_push(v, "x", json_new_position(a.x));
		json_object_push(v, "y", json_new_position(a.y));
		return v;
	} else {
		return json_null_new();
	}
}
otl_Anchor otl_parse_anchor(json_value *v) {
	otl_Anchor anchor = {.present = false, .x = 0, .y = 0};
	if (!v || v->type != json_object) return anchor;
	anchor.present = true;
	anchor.x = json_obj_getnum_fallback(v, "x", 0);
	anchor.y = json_obj_getnum_fallback(v, "y", 0);
	return anchor;
}

bk_Block *bkFromAnchor(otl_Anchor a) {
	if (!a.present) return NULL;
	return bk_new_Block(b16, 1,            // format
	                    b16, (int16_t)a.x, // x
	                    b16, (int16_t)a.y, // y
	                    bkover);
}

// GPOS position value constants
const uint8_t FORMAT_DX = 1;
const uint8_t FORMAT_DY = 2;
const uint8_t FORMAT_DWIDTH = 4;
const uint8_t FORMAT_DHEIGHT = 8;

#define a(n) n + 0, n + 1, n + 1, n + 2             // 2 bits
#define b(n) a(n + 0), a(n + 1), a(n + 1), a(n + 2) // 4 bits
#define c(n) b(n + 0), b(n + 1), b(n + 1), b(n + 2) // 6 bits
#define d(n) c(n + 0), c(n + 1), c(n + 1), c(n + 2) // 8 bits
const uint8_t bits_in[0x100] = {d(0)};
#undef d
#undef c
#undef b
#undef a

// Length of a position value in bytes
uint8_t position_format_length(uint16_t format) {
	return bits_in[format & 0xFF] << 1;
}
otl_PositionValue position_zero() {
	otl_PositionValue v = {0.0, 0.0, 0.0, 0.0};
	return v;
}
// Read a position value from SFNT
otl_PositionValue read_gpos_value(font_file_pointer data, uint32_t tableLength, uint32_t offset,
                                  uint16_t format) {
	otl_PositionValue v = {0.0, 0.0, 0.0, 0.0};
	if (tableLength < offset + position_format_length(format)) return v;
	if (format & FORMAT_DX) { v.dx = read_16s(data + offset), offset += 2; };
	if (format & FORMAT_DY) { v.dy = read_16s(data + offset), offset += 2; };
	if (format & FORMAT_DWIDTH) { v.dWidth = read_16s(data + offset), offset += 2; };
	if (format & FORMAT_DHEIGHT) { v.dHeight = read_16s(data + offset), offset += 2; };
	return v;
}
json_value *gpos_dump_value(otl_PositionValue value) {
	json_value *v = json_object_new(4);
	if (value.dx) json_object_push(v, "dx", json_new_position(value.dx));
	if (value.dy) json_object_push(v, "dy", json_new_position(value.dy));
	if (value.dWidth) json_object_push(v, "dWidth", json_new_position(value.dWidth));
	if (value.dHeight) json_object_push(v, "dHeight", json_new_position(value.dHeight));
	return preserialize(v);
}
otl_PositionValue gpos_parse_value(json_value *pos) {
	otl_PositionValue v = {0.0, 0.0, 0.0, 0.0};
	if (!pos || pos->type != json_object) return v;
	v.dx = json_obj_getnum(pos, "dx");
	v.dy = json_obj_getnum(pos, "dy");
	v.dWidth = json_obj_getnum(pos, "dWidth");
	v.dHeight = json_obj_getnum(pos, "dHeight");
	return v;
}
// The required format of a position value
uint8_t required_position_format(otl_PositionValue v) {
	return (v.dx ? FORMAT_DX : 0) | (v.dy ? FORMAT_DY : 0) | (v.dWidth ? FORMAT_DWIDTH : 0) |
	       (v.dHeight ? FORMAT_DHEIGHT : 0);
}
// Write gpos position value
void write_gpos_value(caryll_Buffer *buf, otl_PositionValue v, uint16_t format) {
	if (format & FORMAT_DX) bufwrite16b(buf, (int16_t)v.dx);
	if (format & FORMAT_DY) bufwrite16b(buf, (int16_t)v.dy);
	if (format & FORMAT_DWIDTH) bufwrite16b(buf, (int16_t)v.dWidth);
	if (format & FORMAT_DHEIGHT) bufwrite16b(buf, (int16_t)v.dHeight);
}

bk_Block *bk_gpos_value(otl_PositionValue v, uint16_t format) {
	bk_Block *b = bk_new_Block(bkover);
	if (format & FORMAT_DX) bk_push(b, b16, (int16_t)v.dx, bkover);
	if (format & FORMAT_DY) bk_push(b, b16, (int16_t)v.dy, bkover);
	if (format & FORMAT_DWIDTH) bk_push(b, b16, (int16_t)v.dWidth, bkover);
	if (format & FORMAT_DHEIGHT) bk_push(b, b16, (int16_t)v.dHeight, bkover);
	return b;
}
