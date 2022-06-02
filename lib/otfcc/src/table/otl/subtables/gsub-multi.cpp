#include "gsub-multi.h"

static void deleteGsubMultiEntry(otl_GsubMultiEntry *entry) {
	Handle.dispose(&entry->from);
	DELETE(Coverage.free, entry->to);
}

static caryll_ElementInterface(otl_GsubMultiEntry) gsm_typeinfo = {
    .init = NULL, .copy = NULL, .dispose = deleteGsubMultiEntry};

caryll_standardVectorImpl(subtable_gsub_multi, otl_GsubMultiEntry, gsm_typeinfo,
                          iSubtable_gsub_multi);

otl_Subtable *otl_read_gsub_multi(font_file_pointer data, uint32_t tableLength, uint32_t offset,
                                  const glyphid_t maxGlyphs, const otfcc_Options *options) {
	subtable_gsub_multi *subtable = iSubtable_gsub_multi.create();
	otl_Coverage *from = NULL;
	checkLength(offset + 6);

	from = Coverage.read(data, tableLength, offset + read_16u(data + offset + 2));
	glyphid_t seqCount = read_16u(data + offset + 4);
	if (seqCount != from->numGlyphs) goto FAIL;
	checkLength(offset + 6 + seqCount * 2);

	for (glyphid_t j = 0; j < seqCount; j++) {
		uint32_t seqOffset = offset + read_16u(data + offset + 6 + j * 2);
		otl_Coverage *cov = Coverage.create();
		glyphid_t n = read_16u(data + seqOffset);
		for (glyphid_t k = 0; k < n; k++) {
			Coverage.push(cov, Handle.fromIndex(read_16u(data + seqOffset + 2 + k * 2)));
		}
		iSubtable_gsub_multi.push(subtable, ((otl_GsubMultiEntry){
		                                        .from = Handle.dup(from->glyphs[j]), .to = cov,
		                                    }));
	}
	Coverage.free(from);
	return (otl_Subtable *)subtable;

FAIL:
	if (from) Coverage.free(from);
	iSubtable_gsub_multi.free(subtable);
	return NULL;
}

json_value *otl_gsub_dump_multi(const otl_Subtable *_subtable) {
	const subtable_gsub_multi *subtable = &(_subtable->gsub_multi);
	json_value *st = json_object_new(subtable->length);
	for (glyphid_t j = 0; j < subtable->length; j++) {
		json_object_push(st, subtable->items[j].from.name, Coverage.dump(subtable->items[j].to));
	}
	return st;
}

otl_Subtable *otl_gsub_parse_multi(const json_value *_subtable, const otfcc_Options *options) {
	subtable_gsub_multi *st = iSubtable_gsub_multi.create();

	for (glyphid_t k = 0; k < _subtable->u.object.length; k++) {
		json_value *_to = _subtable->u.object.values[k].value;
		if (!_to || _to->type != json_array) continue;
		iSubtable_gsub_multi.push(
		    st, ((otl_GsubMultiEntry){
		            .from = Handle.fromName(sdsnewlen(_subtable->u.object.values[k].name,
		                                              _subtable->u.object.values[k].name_length)),
		            .to = Coverage.parse(_to),
		        }));
	}

	return (otl_Subtable *)st;
}

caryll_Buffer *otfcc_build_gsub_multi_subtable(const otl_Subtable *_subtable, otl_BuildHeuristics heuristics) {
	const subtable_gsub_multi *subtable = &(_subtable->gsub_multi);
	otl_Coverage *cov = Coverage.create();
	for (glyphid_t j = 0; j < subtable->length; j++) {
		Coverage.push(cov, Handle.dup(subtable->items[j].from));
	}

	bk_Block *root = bk_new_Block(b16, 1,                                          // format
	                              p16, bk_newBlockFromBuffer(Coverage.build(cov)), // coverage
	                              b16, subtable->length,                           // quantity
	                              bkover);
	for (glyphid_t j = 0; j < subtable->length; j++) {
		bk_Block *b = bk_new_Block(b16, subtable->items[j].to->numGlyphs, bkover);
		for (glyphid_t k = 0; k < subtable->items[j].to->numGlyphs; k++) {
			bk_push(b, b16, subtable->items[j].to->glyphs[k].index, bkover);
		}
		bk_push(root, p16, b, bkover);
	}
	Coverage.free(cov);
	return bk_build_Block(root);
}
