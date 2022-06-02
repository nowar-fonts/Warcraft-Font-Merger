#include "gsub-reverse.h"

static INLINE void initGsubReverse(subtable_gsub_reverse *subtable) {
	subtable->match = NULL;
	subtable->to = NULL;
}
static INLINE void disposeGsubReverse(subtable_gsub_reverse *subtable) {
	if (subtable->match)
		for (tableid_t j = 0; j < subtable->matchCount; j++) {
			Coverage.free(subtable->match[j]);
		}
	if (subtable->to) Coverage.free(subtable->to);
}

caryll_standardRefType(subtable_gsub_reverse, iSubtable_gsub_reverse, initGsubReverse,
                       disposeGsubReverse);

static void reverseBacktracks(otl_Coverage **match, tableid_t inputIndex) {
	if (inputIndex > 0) {
		tableid_t start = 0;
		tableid_t end = inputIndex - 1;
		while (end > start) {
			otl_Coverage *tmp = match[start];
			match[start] = match[end];
			match[end] = tmp;
			end--, start++;
		}
	}
}

otl_Subtable *otl_read_gsub_reverse(const font_file_pointer data, uint32_t tableLength,
                                    uint32_t offset, const glyphid_t maxGlyphs,
                                    const otfcc_Options *options) {
	subtable_gsub_reverse *subtable = iSubtable_gsub_reverse.create();
	checkLength(offset + 6);

	tableid_t nBacktrack = read_16u(data + offset + 4);
	checkLength(offset + 6 + nBacktrack * 2);

	tableid_t nForward = read_16u(data + offset + 6 + nBacktrack * 2);
	checkLength(offset + 8 + (nBacktrack + nForward) * 2);

	tableid_t nReplacement = read_16u(data + offset + 8 + (nBacktrack + nForward) * 2);
	checkLength(offset + 10 + (nBacktrack + nForward + nReplacement) * 2);

	subtable->matchCount = nBacktrack + nForward + 1;
	NEW(subtable->match, subtable->matchCount);
	subtable->inputIndex = nBacktrack;

	for (tableid_t j = 0; j < nBacktrack; j++) {
		uint32_t covOffset = offset + read_16u(data + offset + 6 + j * 2);
		subtable->match[j] = Coverage.read(data, tableLength, covOffset);
	}
	{
		uint32_t covOffset = offset + read_16u(data + offset + 2);
		subtable->match[subtable->inputIndex] = Coverage.read(data, tableLength, covOffset);
		if (nReplacement != subtable->match[subtable->inputIndex]->numGlyphs) goto FAIL;
	}
	for (tableid_t j = 0; j < nForward; j++) {
		uint32_t covOffset = offset + read_16u(data + offset + 8 + nBacktrack * 2 + j * 2);
		subtable->match[nBacktrack + 1 + j] = Coverage.read(data, tableLength, covOffset);
	}

	NEW(subtable->to);
	subtable->to->numGlyphs = nReplacement;
	NEW(subtable->to->glyphs, nReplacement);
	for (tableid_t j = 0; j < nReplacement; j++) {
		subtable->to->glyphs[j] =
		    Handle.fromIndex(read_16u(data + offset + 10 + (nBacktrack + nForward + j) * 2));
	}
	reverseBacktracks(subtable->match, subtable->inputIndex);
	return (otl_Subtable *)subtable;

FAIL:
	iSubtable_gsub_reverse.free(subtable);
	return NULL;
}

json_value *otl_gsub_dump_reverse(const otl_Subtable *_subtable) {
	const subtable_gsub_reverse *subtable = &(_subtable->gsub_reverse);
	json_value *_st = json_object_new(3);
	json_value *_match = json_array_new(subtable->matchCount);
	for (tableid_t j = 0; j < subtable->matchCount; j++) {
		json_array_push(_match, Coverage.dump(subtable->match[j]));
	}
	json_object_push(_st, "match", _match);
	json_object_push(_st, "to", Coverage.dump(subtable->to));
	json_object_push(_st, "inputIndex", json_integer_new(subtable->inputIndex));
	return _st;
}

otl_Subtable *otl_gsub_parse_reverse(const json_value *_subtable, const otfcc_Options *options) {
	json_value *_match = json_obj_get_type(_subtable, "match", json_array);
	json_value *_to = json_obj_get_type(_subtable, "to", json_array);
	if (!_match || !_to) return NULL;

	subtable_gsub_reverse *subtable = iSubtable_gsub_reverse.create();

	subtable->matchCount = _match->u.array.length;
	NEW(subtable->match, subtable->matchCount);

	subtable->inputIndex = json_obj_getnum_fallback(_subtable, "inputIndex", 0);

	for (tableid_t j = 0; j < subtable->matchCount; j++) {
		subtable->match[j] = Coverage.parse(_match->u.array.values[j]);
	}
	subtable->to = Coverage.parse(_to);
	return (otl_Subtable *)subtable;
}

caryll_Buffer *otfcc_build_gsub_reverse(const otl_Subtable *_subtable, otl_BuildHeuristics heuristics) {
	const subtable_gsub_reverse *subtable = &(_subtable->gsub_reverse);
	reverseBacktracks(subtable->match, subtable->inputIndex);

	bk_Block *root = bk_new_Block(b16, 1, // format
	                              p16, bk_newBlockFromBuffer(Coverage.build(
	                                       subtable->match[subtable->inputIndex])), // coverage
	                              bkover);
	bk_push(root, b16, subtable->inputIndex, bkover);
	for (tableid_t j = 0; j < subtable->inputIndex; j++) {
		bk_push(root, p16, bk_newBlockFromBuffer(Coverage.build(subtable->match[j])), bkover);
	}
	bk_push(root, b16, subtable->matchCount - subtable->inputIndex - 1, bkover);
	for (tableid_t j = subtable->inputIndex + 1; j < subtable->matchCount; j++) {
		bk_push(root, p16, bk_newBlockFromBuffer(Coverage.build(subtable->match[j])), bkover);
	}
	bk_push(root, b16, subtable->to->numGlyphs, bkover);
	for (tableid_t j = 0; j < subtable->to->numGlyphs; j++) {
		bk_push(root, b16, subtable->to->glyphs[j].index, bkover);
	}

	return bk_build_Block(root);
}
