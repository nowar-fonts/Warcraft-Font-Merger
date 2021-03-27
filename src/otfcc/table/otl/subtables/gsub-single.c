#include "gsub-single.h"

static void gss_entry_ctor(MODIFY otl_GsubSingleEntry *entry) {
	entry->from = Handle.empty();
	entry->to = Handle.empty();
}
static void gss_entry_copyctor(MODIFY otl_GsubSingleEntry *dst,
                               COPY const otl_GsubSingleEntry *src) {
	dst->from = Handle.dup(src->from);
	dst->to = Handle.dup(src->to);
}
static void gss_entry_dtor(MODIFY otl_GsubSingleEntry *entry) {
	Handle.dispose(&entry->from);
	Handle.dispose(&entry->to);
}

static caryll_ElementInterface(otl_GsubSingleEntry) gss_typeinfo = {
    .init = gss_entry_ctor, .copy = gss_entry_copyctor, .dispose = gss_entry_dtor};

caryll_standardVectorImpl(subtable_gsub_single, otl_GsubSingleEntry, gss_typeinfo,
                          iSubtable_gsub_single);

otl_Subtable *otl_read_gsub_single(const font_file_pointer data, uint32_t tableLength,
                                   uint32_t subtableOffset, const glyphid_t maxGlyphs,
                                   const otfcc_Options *options) {
	subtable_gsub_single *subtable = iSubtable_gsub_single.create();
	otl_Coverage *from = NULL;
	otl_Coverage *to = NULL;
	if (tableLength < subtableOffset + 6) goto FAIL;

	uint16_t subtableFormat = read_16u(data + subtableOffset);
	from = Coverage.read(data, tableLength, subtableOffset + read_16u(data + subtableOffset + 2));
	if (!from || from->numGlyphs == 0) goto FAIL;

	if (subtableFormat == 1) {
		NEW(to);
		to->numGlyphs = from->numGlyphs;
		NEW(to->glyphs, to->numGlyphs);

		uint16_t delta = read_16u(data + subtableOffset + 4);
		for (glyphid_t j = 0; j < from->numGlyphs; j++) {
			to->glyphs[j] = Handle.fromIndex(from->glyphs[j].index + delta);
		}
	} else {
		glyphid_t toglyphs = read_16u(data + subtableOffset + 4);
		if (tableLength < subtableOffset + 6 + toglyphs * 2 || toglyphs != from->numGlyphs)
			goto FAIL;
		NEW(to);
		to->numGlyphs = toglyphs;
		NEW(to->glyphs, to->numGlyphs);

		for (glyphid_t j = 0; j < to->numGlyphs; j++) {
			to->glyphs[j] = Handle.fromIndex(read_16u(data + subtableOffset + 6 + j * 2));
		}
	}
	goto OK;
FAIL:
	iSubtable_gsub_single.free(subtable);
	if (from) Coverage.free(from);
	if (to) Coverage.free(to);
	return NULL;
OK:
	for (glyphid_t j = 0; j < from->numGlyphs; j++) {
		iSubtable_gsub_single.push(subtable, ((otl_GsubSingleEntry){
		                                         .from = Handle.dup(from->glyphs[j]), // from
		                                         .to = Handle.dup(to->glyphs[j]),     // to
		                                     }));
	}
	if (from) Coverage.free(from);
	if (to) Coverage.free(to);
	return (otl_Subtable *)subtable;
}

json_value *otl_gsub_dump_single(const otl_Subtable *_subtable) {
	const subtable_gsub_single *subtable = &(_subtable->gsub_single);
	json_value *st = json_object_new(subtable->length);
	for (size_t j = 0; j < subtable->length; j++) {
		json_object_push(st, subtable->items[j].from.name,
		                 json_string_new(subtable->items[j].to.name));
	}
	return st;
}

otl_Subtable *otl_gsub_parse_single(const json_value *_subtable, const otfcc_Options *options) {
	subtable_gsub_single *subtable = iSubtable_gsub_single.create();
	for (glyphid_t j = 0; j < _subtable->u.object.length; j++) {
		if (_subtable->u.object.values[j].value &&
		    _subtable->u.object.values[j].value->type == json_string) {
			glyph_handle from = Handle.fromName(sdsnewlen(
			    _subtable->u.object.values[j].name, _subtable->u.object.values[j].name_length));
			glyph_handle to =
			    Handle.fromName(sdsnewlen(_subtable->u.object.values[j].value->u.string.ptr,
			                              _subtable->u.object.values[j].value->u.string.length));
			iSubtable_gsub_single.push(subtable, ((otl_GsubSingleEntry){.from = from, .to = to}));
		}
	}
	return (otl_Subtable *)subtable;
};

caryll_Buffer *otfcc_build_gsub_single_subtable(const otl_Subtable *_subtable, otl_BuildHeuristics heuristics) {
	const subtable_gsub_single *subtable = &(_subtable->gsub_single);
	bool isConstantDifference = subtable->length > 0;
	if (isConstantDifference) {
		int32_t difference = subtable->items[0].to.index - subtable->items[0].from.index;
		isConstantDifference = isConstantDifference && difference < 0x8000 && difference > -0x8000;
		for (glyphid_t j = 1; j < subtable->length; j++) {
			int32_t diffJ = subtable->items[j].to.index - subtable->items[j].from.index;
			isConstantDifference = isConstantDifference && diffJ == difference
				&& diffJ < 0x8000 && diffJ > -0x8000;
		}
	}
	otl_Coverage *cov = Coverage.create();
	for (glyphid_t j = 0; j < subtable->length; j++) {
		Coverage.push(cov, Handle.dup(subtable->items[j].from));
	}

	caryll_Buffer *coverageBuf = Coverage.buildFormat(cov, heuristics & OTL_BH_GSUB_VERT ? 1 : 0);

	if (isConstantDifference && !(heuristics & OTL_BH_GSUB_VERT)) {
		bk_Block *b =
		    bk_new_Block(b16, 1,                                          // Format
		                 p16, bk_newBlockFromBuffer(coverageBuf),         // coverage
		                 b16,
		                 subtable->items[0].to.index - subtable->items[0].from.index, // delta
		                 bkover);
		Coverage.free(cov);
		return bk_build_Block(b);
	} else {
		bk_Block *b = bk_new_Block(b16, 2,                                          // Format
		                           p16, bk_newBlockFromBuffer(coverageBuf),         // coverage
		                           b16, subtable->length,                           // quantity
		                           bkover);
		for (glyphid_t k = 0; k < subtable->length; k++) {
			bk_push(b, b16, subtable->items[k].to.index, bkover);
		}
		Coverage.free(cov);
		return bk_build_Block(b);
	}
}
