#include "gpos-single.h"
#include "gpos-common.h"

static void deleteGposSingleEntry(otl_GposSingleEntry *entry) {
	Handle.dispose(&entry->target);
}

static caryll_ElementInterface(otl_GposSingleEntry) gss_typeinfo = {
    .init = NULL, .copy = NULL, .dispose = deleteGposSingleEntry};

caryll_standardVectorImpl(subtable_gpos_single, otl_GposSingleEntry, gss_typeinfo,
                          iSubtable_gpos_single);

otl_Subtable *otl_read_gpos_single(const font_file_pointer data, uint32_t tableLength,
                                   uint32_t offset, const glyphid_t maxGlyphs,
                                   const otfcc_Options *options) {
	subtable_gpos_single *subtable = iSubtable_gpos_single.create();
	otl_Coverage *targets = NULL;

	checkLength(offset + 6);

	uint16_t subtableFormat = read_16u(data + offset);
	targets = Coverage.read(data, tableLength, offset + read_16u(data + offset + 2));
	if (!targets || targets->numGlyphs == 0) goto FAIL;

	if (subtableFormat == 1) {
		otl_PositionValue v =
		    read_gpos_value(data, tableLength, offset + 6, read_16u(data + offset + 4));
		for (glyphid_t j = 0; j < targets->numGlyphs; j++) {
			iSubtable_gpos_single.push(subtable,
			                           ((otl_GposSingleEntry){
			                               .target = Handle.dup(targets->glyphs[j]), .value = v,
			                           }));
		}
	} else {
		uint16_t valueFormat = read_16u(data + offset + 4);
		uint16_t valueCount = read_16u(data + offset + 6);
		checkLength(offset + 8 + position_format_length(valueFormat) * valueCount);
		if (valueCount != targets->numGlyphs) goto FAIL;

		for (glyphid_t j = 0; j < targets->numGlyphs; j++) {
			iSubtable_gpos_single.push(
			    subtable,
			    ((otl_GposSingleEntry){
			        .target = Handle.dup(targets->glyphs[j]),
			        .value = read_gpos_value(data, tableLength,
			                                 offset + 8 + j * position_format_length(valueFormat),
			                                 valueFormat),
			    }));
		}
	}
	if (targets) Coverage.free(targets);
	return (otl_Subtable *)subtable;

FAIL:
	if (targets) Coverage.free(targets);
	iSubtable_gpos_single.free(subtable);
	return NULL;
}

json_value *otl_gpos_dump_single(const otl_Subtable *_subtable) {
	const subtable_gpos_single *subtable = &(_subtable->gpos_single);
	json_value *st = json_object_new(subtable->length);
	for (glyphid_t j = 0; j < subtable->length; j++) {
		json_object_push(st, subtable->items[j].target.name,
		                 gpos_dump_value(subtable->items[j].value));
	}
	return st;
}
otl_Subtable *otl_gpos_parse_single(const json_value *_subtable, const otfcc_Options *options) {
	subtable_gpos_single *subtable = iSubtable_gpos_single.create();
	for (glyphid_t j = 0; j < _subtable->u.object.length; j++) {
		if (_subtable->u.object.values[j].value &&
		    _subtable->u.object.values[j].value->type == json_object) {
			sds gname = sdsnewlen(_subtable->u.object.values[j].name,
			                      _subtable->u.object.values[j].name_length);
			iSubtable_gpos_single.push(
			    subtable, ((otl_GposSingleEntry){
			                  .target = Handle.fromName(gname),
			                  .value = gpos_parse_value(_subtable->u.object.values[j].value),
			              }));
		}
	}
	return (otl_Subtable *)subtable;
}

caryll_Buffer *otfcc_build_gpos_single(const otl_Subtable *_subtable, otl_BuildHeuristics heuristics) {
	const subtable_gpos_single *subtable = &(_subtable->gpos_single);
	bool isConst = subtable->length > 0;
	uint16_t format = 0;
	if (subtable->length > 0) {
		for (glyphid_t j = 0; j < subtable->length; j++) {
			isConst = isConst && (subtable->items[j].value.dx == subtable->items[0].value.dx) &&
			          (subtable->items[j].value.dy == subtable->items[0].value.dy) &&
			          (subtable->items[j].value.dWidth == subtable->items[0].value.dWidth) &&
			          (subtable->items[j].value.dHeight == subtable->items[0].value.dHeight);
			format |= required_position_format(subtable->items[j].value);
		}
	}
	otl_Coverage *cov = Coverage.create();
	for (glyphid_t j = 0; j < subtable->length; j++) {
		Coverage.push(cov, Handle.dup(subtable->items[j].target));
	}

	caryll_Buffer *coverageBuf = Coverage.build(cov);
	
	if (isConst) {
		bk_Block *b =
		    (bk_new_Block(b16, 1, // Format
		                  p16, bk_newBlockFromBuffer(coverageBuf),                  // coverage
		                  b16, format,                                              // format
		                  bkembed, bk_gpos_value(subtable->items[0].value, format), // value
		                  bkover));
		Coverage.free(cov);
		return bk_build_Block(b);
	} else {
		bk_Block *b = bk_new_Block(b16, 2,                                          // Format
		                           p16, bk_newBlockFromBuffer(coverageBuf),         // coverage
		                           b16, format,                                     // format
		                           b16, subtable->length,                           // quantity
		                           bkover);
		for (glyphid_t k = 0; k < subtable->length; k++) {
			bk_push(b, bkembed, bk_gpos_value(subtable->items[k].value, format), // value
			        bkover);
		}
		Coverage.free(cov);
		return bk_build_Block(b);
	}
}
