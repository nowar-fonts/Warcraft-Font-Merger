#include "gpos-cursive.h"
#include "gpos-common.h"

static void deleteGposCursiveEntry(otl_GposCursiveEntry *entry) {
	Handle.dispose(&entry->target);
}

static caryll_ElementInterface(otl_GposCursiveEntry) gss_typeinfo = {
    .init = NULL, .copy = NULL, .dispose = deleteGposCursiveEntry};

caryll_standardVectorImpl(subtable_gpos_cursive, otl_GposCursiveEntry, gss_typeinfo,
                          iSubtable_gpos_cursive);

otl_Subtable *otl_read_gpos_cursive(const font_file_pointer data, uint32_t tableLength,
                                    uint32_t offset, const glyphid_t maxGlyphs,
                                    const otfcc_Options *options) {
	subtable_gpos_cursive *subtable = iSubtable_gpos_cursive.create();
	otl_Coverage *targets = NULL;

	checkLength(offset + 6);

	targets = Coverage.read(data, tableLength, offset + read_16u(data + offset + 2));
	if (!targets || targets->numGlyphs == 0) goto FAIL;

	glyphid_t valueCount = read_16u(data + offset + 4);
	checkLength(offset + 6 + 4 * valueCount);
	if (valueCount != targets->numGlyphs) goto FAIL;

	for (glyphid_t j = 0; j < valueCount; j++) {
		uint16_t enterOffset = read_16u(data + offset + 6 + 4 * j);
		uint16_t exitOffset = read_16u(data + offset + 6 + 4 * j + 2);
		otl_Anchor enter = otl_anchor_absent();
		otl_Anchor exit = otl_anchor_absent();
		if (enterOffset) { enter = otl_read_anchor(data, tableLength, offset + enterOffset); }
		if (exitOffset) { exit = otl_read_anchor(data, tableLength, offset + exitOffset); }
		iSubtable_gpos_cursive.push(
		    subtable, ((otl_GposCursiveEntry){
		                  .target = Handle.dup(targets->glyphs[j]), .enter = enter, .exit = exit}));
	}
	if (targets) Coverage.free(targets);
	return (otl_Subtable *)subtable;
FAIL:
	if (targets) Coverage.free(targets);
	iSubtable_gpos_cursive.free(subtable);
	return NULL;
}

json_value *otl_gpos_dump_cursive(const otl_Subtable *_subtable) {
	const subtable_gpos_cursive *subtable = &(_subtable->gpos_cursive);
	json_value *st = json_object_new(subtable->length);
	for (glyphid_t j = 0; j < subtable->length; j++) {
		json_value *rec = json_object_new(2);
		json_object_push(rec, "enter", otl_dump_anchor(subtable->items[j].enter));
		json_object_push(rec, "exit", otl_dump_anchor(subtable->items[j].exit));
		json_object_push(st, subtable->items[j].target.name, preserialize(rec));
	}
	return st;
}

otl_Subtable *otl_gpos_parse_cursive(const json_value *_subtable, const otfcc_Options *options) {
	subtable_gpos_cursive *subtable = iSubtable_gpos_cursive.create();
	for (glyphid_t j = 0; j < _subtable->u.object.length; j++) {
		if (_subtable->u.object.values[j].value &&
		    _subtable->u.object.values[j].value->type == json_object) {
			sds gname = sdsnewlen(_subtable->u.object.values[j].name,
			                      _subtable->u.object.values[j].name_length);
			iSubtable_gpos_cursive.push(
			    subtable, ((otl_GposCursiveEntry){
			                  .target = Handle.fromName(gname),
			                  .enter = otl_parse_anchor(
			                      json_obj_get(_subtable->u.object.values[j].value, "enter")),
			                  .exit = otl_parse_anchor(
			                      json_obj_get(_subtable->u.object.values[j].value, "exit")),
			              }));
		}
	}
	return (otl_Subtable *)subtable;
}

caryll_Buffer *otfcc_build_gpos_cursive(const otl_Subtable *_subtable,
                                        otl_BuildHeuristics heuristics) {
	const subtable_gpos_cursive *subtable = &(_subtable->gpos_cursive);
	otl_Coverage *cov = Coverage.create();
	for (glyphid_t j = 0; j < subtable->length; j++) {
		Coverage.push(cov, Handle.dup(subtable->items[j].target));
	}

	bk_Block *root = bk_new_Block(b16, 1,                                          // format
	                              p16, bk_newBlockFromBuffer(Coverage.build(cov)), // Coverage
	                              b16, subtable->length,                           // EntryExitCount
	                              bkover);
	for (glyphid_t j = 0; j < subtable->length; j++) {
		bk_push(root,                                        // EntryExitRecord[.]
		        p16, bkFromAnchor(subtable->items[j].enter), // enter
		        p16, bkFromAnchor(subtable->items[j].exit),  // exit
		        bkover);
	}
	Coverage.free(cov);

	return bk_build_Block(root);
}
