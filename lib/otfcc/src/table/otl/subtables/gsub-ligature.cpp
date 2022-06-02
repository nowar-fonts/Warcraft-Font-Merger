#include "gsub-ligature.h"

static void deleteGsubLigatureEntry(otl_GsubLigatureEntry *entry) {
	Handle.dispose(&entry->to);
	DELETE(Coverage.free, entry->from);
}
static caryll_ElementInterface(otl_GsubLigatureEntry) gss_typeinfo = {
    .init = NULL, .copy = NULL, .dispose = deleteGsubLigatureEntry};

caryll_standardVectorImpl(subtable_gsub_ligature, otl_GsubLigatureEntry, gss_typeinfo,
                          iSubtable_gsub_ligature);

otl_Subtable *otl_read_gsub_ligature(const font_file_pointer data, uint32_t tableLength,
                                     uint32_t offset, const glyphid_t maxGlyphs,
                                     const otfcc_Options *options) {
	subtable_gsub_ligature *subtable = iSubtable_gsub_ligature.create();
	checkLength(offset + 6);

	otl_Coverage *startCoverage =
	    Coverage.read(data, tableLength, offset + read_16u(data + offset + 2));
	if (!startCoverage) goto FAIL;
	glyphid_t setCount = read_16u(data + offset + 4);
	if (setCount != startCoverage->numGlyphs) goto FAIL;
	checkLength(offset + 6 + setCount * 2);

	uint32_t ligatureCount = 0;
	for (glyphid_t j = 0; j < setCount; j++) {
		uint32_t setOffset = offset + read_16u(data + offset + 6 + j * 2);
		checkLength(setOffset + 2);
		ligatureCount += read_16u(data + setOffset);
		checkLength(setOffset + 2 + read_16u(data + setOffset) * 2);
	}

	for (glyphid_t j = 0; j < setCount; j++) {
		uint32_t setOffset = offset + read_16u(data + offset + 6 + j * 2);
		glyphid_t lc = read_16u(data + setOffset);
		for (glyphid_t k = 0; k < lc; k++) {
			uint32_t ligOffset = setOffset + read_16u(data + setOffset + 2 + k * 2);
			checkLength(ligOffset + 4);
			glyphid_t ligComponents = read_16u(data + ligOffset + 2);
			checkLength(ligOffset + 2 + ligComponents * 2);

			otl_Coverage *cov = Coverage.create();
			Coverage.push(cov, Handle.fromIndex(startCoverage->glyphs[j].index));
			for (glyphid_t m = 1; m < ligComponents; m++) {
				Coverage.push(cov, Handle.fromIndex(read_16u(data + ligOffset + 2 + m * 2)));
			}
			iSubtable_gsub_ligature.push(
			    subtable, ((otl_GsubLigatureEntry){
			                  .from = cov, .to = Handle.fromIndex(read_16u(data + ligOffset)),
			              }));
		}
	}
	Coverage.free(startCoverage);
	return (otl_Subtable *)subtable;
FAIL:
	iSubtable_gsub_ligature.free(subtable);
	return NULL;
}

json_value *otl_gsub_dump_ligature(const otl_Subtable *_subtable) {
	const subtable_gsub_ligature *subtable = &(_subtable->gsub_ligature);
	json_value *st = json_array_new(subtable->length);
	for (glyphid_t j = 0; j < subtable->length; j++) {
		json_value *entry = json_object_new(2);
		json_object_push(entry, "from", Coverage.dump(subtable->items[j].from));
		json_object_push(entry, "to",
		                 json_string_new_length((uint32_t)sdslen(subtable->items[j].to.name),
		                                        subtable->items[j].to.name));
		json_array_push(st, preserialize(entry));
	}
	json_value *ret = json_object_new(1);
	json_object_push(ret, "substitutions", st);
	return ret;
}

otl_Subtable *otl_gsub_parse_ligature(const json_value *_subtable, const otfcc_Options *options) {
	if (json_obj_get_type(_subtable, "substitutions", json_array)) {
		_subtable = json_obj_get_type(_subtable, "substitutions", json_array);

		subtable_gsub_ligature *st = iSubtable_gsub_ligature.create();
		glyphid_t n = _subtable->u.array.length;

		for (glyphid_t k = 0; k < n; k++) {
			json_value *entry = _subtable->u.array.values[k];
			json_value *_from = json_obj_get_type(entry, "from", json_array);
			json_value *_to = json_obj_get_type(entry, "to", json_string);
			if (!_from || !_to) continue;
			iSubtable_gsub_ligature.push(
			    st, ((otl_GsubLigatureEntry){
			            .to = Handle.fromName(sdsnewlen(_to->u.string.ptr, _to->u.string.length)),
			            .from = Coverage.parse(_from),
			        }));
		}
		return (otl_Subtable *)st;
	} else {
		subtable_gsub_ligature *st = iSubtable_gsub_ligature.create();
		glyphid_t n = _subtable->u.array.length;

		for (glyphid_t k = 0; k < n; k++) {
			json_value *_from = _subtable->u.object.values[k].value;
			if (!_from || _from->type != json_array) continue;
			iSubtable_gsub_ligature.push(
			    st, ((otl_GsubLigatureEntry){
			            .to = Handle.fromName(sdsnewlen(_subtable->u.object.values[k].name,
			                                            _subtable->u.object.values[k].name_length)),
			            .from = Coverage.parse(_from),
			        }));
		}
		return (otl_Subtable *)st;
	}
	return NULL;
}

typedef struct {
	int gid;
	UT_hash_handle hh;
} ligature_aggerator;
static int by_gid(ligature_aggerator *a, ligature_aggerator *b) {
	return a->gid - b->gid;
}

caryll_Buffer *otfcc_build_gsub_ligature_subtable(const otl_Subtable *_subtable, otl_BuildHeuristics heuristics) {
	const subtable_gsub_ligature *subtable = &(_subtable->gsub_ligature);

	ligature_aggerator *h = NULL, *s, *tmp;
	glyphid_t nLigatures = subtable->length;
	for (glyphid_t j = 0; j < nLigatures; j++) {
		int sgid = subtable->items[j].from->glyphs[0].index;
		HASH_FIND_INT(h, &sgid, s);
		if (!s) {
			NEW(s);
			s->gid = sgid;
			HASH_ADD_INT(h, gid, s);
		}
	}
	HASH_SORT(h, by_gid);

	otl_Coverage *startcov = Coverage.create();

	foreach_hash(s, h) {
		Coverage.push(startcov, Handle.fromIndex(s->gid));
	}

	bk_Block *root = bk_new_Block(b16, 1,                                               // format
	                              p16, bk_newBlockFromBuffer(Coverage.build(startcov)), // coverage
	                              b16, startcov->numGlyphs, // LigSetCount
	                              bkover);

	foreach_hash(s, h) {
		glyphid_t nLigsHere = 0;
		for (glyphid_t j = 0; j < nLigatures; j++)
			if (subtable->items[j].from->glyphs[0].index == s->gid) nLigsHere++;
		bk_Block *ligset = bk_new_Block(b16, nLigsHere, bkover);

		for (glyphid_t j = 0; j < nLigatures; j++) {
			if (subtable->items[j].from->glyphs[0].index == s->gid) {
				bk_Block *ligdef =
				    bk_new_Block(b16, subtable->items[j].to.index,        // ligGlyph
				                 b16, subtable->items[j].from->numGlyphs, // compCount
				                 bkover);
				for (glyphid_t m = 1; m < subtable->items[j].from->numGlyphs; m++) {
					bk_push(ligdef, b16, subtable->items[j].from->glyphs[m].index, bkover);
				}
				bk_push(ligset, p16, ligdef, bkover);
			}
		}
		bk_push(root, p16, ligset, bkover);
	}

	Coverage.free(startcov);
	HASH_ITER(hh, h, s, tmp) {
		HASH_DEL(h, s);
		FREE(s);
	}
	return bk_build_Block(root);
}
