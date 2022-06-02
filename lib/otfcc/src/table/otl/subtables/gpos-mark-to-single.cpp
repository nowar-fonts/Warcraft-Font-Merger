#include "gpos-mark-to-single.h"
#include "gpos-common.h"

static void deleteBaseArrayItem(otl_BaseRecord *entry) {
	Handle.dispose(&entry->glyph);
	FREE(entry->anchors);
}
static caryll_ElementInterface(otl_BaseRecord) ba_typeinfo = {
    .init = NULL, .copy = NULL, .dispose = deleteBaseArrayItem};

caryll_standardVectorImpl(otl_BaseArray, otl_BaseRecord, ba_typeinfo, otl_iBaseArray);

static INLINE void initMarkToSingle(subtable_gpos_markToSingle *subtable) {
	otl_iMarkArray.init(&subtable->markArray);
	otl_iBaseArray.init(&subtable->baseArray);
}
static INLINE void disposeMarkToSingle(subtable_gpos_markToSingle *subtable) {
	otl_iMarkArray.dispose(&subtable->markArray);
	otl_iBaseArray.dispose(&subtable->baseArray);
}

caryll_standardRefType(subtable_gpos_markToSingle, iSubtable_gpos_markToSingle, initMarkToSingle,
                       disposeMarkToSingle);

otl_Subtable *otl_read_gpos_markToSingle(const font_file_pointer data, uint32_t tableLength,
                                         uint32_t subtableOffset, const glyphid_t maxGlyphs,
                                         const otfcc_Options *options) {

	subtable_gpos_markToSingle *subtable = iSubtable_gpos_markToSingle.create();
	otl_Coverage *marks = NULL;
	otl_Coverage *bases = NULL;

	if (tableLength < subtableOffset + 12) goto FAIL;
	marks = Coverage.read(data, tableLength, subtableOffset + read_16u(data + subtableOffset + 2));
	bases = Coverage.read(data, tableLength, subtableOffset + read_16u(data + subtableOffset + 4));
	if (!marks || marks->numGlyphs == 0 || !bases || bases->numGlyphs == 0) goto FAIL;

	subtable->classCount = read_16u(data + subtableOffset + 6);
	uint32_t markArrayOffset = subtableOffset + read_16u(data + subtableOffset + 8);
	otl_readMarkArray(&subtable->markArray, marks, data, tableLength, markArrayOffset);

	uint32_t baseArrayOffset = subtableOffset + read_16u(data + subtableOffset + 10);
	checkLength(baseArrayOffset + 2 + 2 * bases->numGlyphs * subtable->classCount);
	if (read_16u(data + baseArrayOffset) != bases->numGlyphs) goto FAIL;

	uint32_t _offset = baseArrayOffset + 2;
	for (glyphid_t j = 0; j < bases->numGlyphs; j++) {
		otl_Anchor *baseAnchors;
		NEW(baseAnchors, subtable->classCount);
		for (glyphclass_t k = 0; k < subtable->classCount; k++) {
			if (read_16u(data + _offset)) {
				baseAnchors[k] =
				    otl_read_anchor(data, tableLength, baseArrayOffset + read_16u(data + _offset));
			} else {
				baseAnchors[k] = otl_anchor_absent();
			}
			_offset += 2;
		}
		otl_iBaseArray.push(
		    &subtable->baseArray,
		    ((otl_BaseRecord){.glyph = Handle.dup(bases->glyphs[j]), .anchors = baseAnchors}));
	}
	if (marks) Coverage.free(marks);
	if (bases) Coverage.free(bases);
	return (otl_Subtable *)subtable;
FAIL:
	iSubtable_gpos_markToSingle.free(subtable);
	return NULL;
}

json_value *otl_gpos_dump_markToSingle(const otl_Subtable *st) {
	const subtable_gpos_markToSingle *subtable = &(st->gpos_markToSingle);
	json_value *_subtable = json_object_new(3);
	json_value *_marks = json_object_new(subtable->markArray.length);
	json_value *_bases = json_object_new(subtable->baseArray.length);
	for (glyphid_t j = 0; j < subtable->markArray.length; j++) {
		json_value *_mark = json_object_new(3);
		sds markClassName =
		    sdscatfmt(sdsempty(), "anchor%i", subtable->markArray.items[j].markClass);
		json_object_push(_mark, "class",
		                 json_string_new_length((uint32_t)sdslen(markClassName), markClassName));
		sdsfree(markClassName);
		json_object_push(_mark, "x", json_integer_new(subtable->markArray.items[j].anchor.x));
		json_object_push(_mark, "y", json_integer_new(subtable->markArray.items[j].anchor.y));
		json_object_push(_marks, subtable->markArray.items[j].glyph.name, preserialize(_mark));
	}
	for (glyphid_t j = 0; j < subtable->baseArray.length; j++) {
		json_value *_base = json_object_new(subtable->classCount);
		for (glyphclass_t k = 0; k < subtable->classCount; k++) {
			if (subtable->baseArray.items[j].anchors[k].present) {
				json_value *_anchor = json_object_new(2);
				json_object_push(_anchor, "x",
				                 json_integer_new(subtable->baseArray.items[j].anchors[k].x));
				json_object_push(_anchor, "y",
				                 json_integer_new(subtable->baseArray.items[j].anchors[k].y));
				sds markClassName = sdscatfmt(sdsempty(), "anchor%i", k);
				json_object_push_length(_base, (uint32_t)sdslen(markClassName), markClassName,
				                        _anchor);
				sdsfree(markClassName);
			}
		}
		json_object_push(_bases, subtable->baseArray.items[j].glyph.name, preserialize(_base));
	}
	json_object_push(_subtable, "marks", _marks);
	json_object_push(_subtable, "bases", _bases);
	return _subtable;
}

static void parseBases(json_value *_bases, subtable_gpos_markToSingle *subtable,
                       otl_ClassnameHash **h, const otfcc_Options *options) {
	glyphclass_t classCount = HASH_COUNT(*h);
	for (glyphid_t j = 0; j < _bases->u.object.length; j++) {
		char *gname = _bases->u.object.values[j].name;
		otl_BaseRecord base;
		base.glyph = Handle.fromName(sdsnewlen(gname, _bases->u.object.values[j].name_length));
		NEW(base.anchors, classCount);
		for (glyphclass_t k = 0; k < classCount; k++) {
			base.anchors[k] = otl_anchor_absent();
		}
		json_value *baseRecord = _bases->u.object.values[j].value;
		if (!baseRecord || baseRecord->type != json_object) {
			otl_iBaseArray.push(&subtable->baseArray, base);
			continue;
		}

		for (glyphclass_t k = 0; k < baseRecord->u.object.length; k++) {
			sds className = sdsnewlen(baseRecord->u.object.values[k].name,
			                          baseRecord->u.object.values[k].name_length);
			otl_ClassnameHash *s;
			HASH_FIND_STR(*h, className, s);
			if (!s) {
				logWarning("[OTFCC-fea] Invalid anchor class name <%s> for /%s. This base anchor "
				           "is ignored.\n",
				           className, gname);
				goto NEXT;
			}
			base.anchors[s->classID] = otl_parse_anchor(baseRecord->u.object.values[k].value);
		NEXT:
			sdsfree(className);
		}
		otl_iBaseArray.push(&subtable->baseArray, base);
	}
}
otl_Subtable *otl_gpos_parse_markToSingle(const json_value *_subtable,
                                          const otfcc_Options *options) {
	json_value *_marks = json_obj_get_type(_subtable, "marks", json_object);
	json_value *_bases = json_obj_get_type(_subtable, "bases", json_object);
	if (!_marks || !_bases) return NULL;
	subtable_gpos_markToSingle *st = iSubtable_gpos_markToSingle.create();
	otl_ClassnameHash *h = NULL;
	otl_parseMarkArray(_marks, &st->markArray, &h, options);
	st->classCount = HASH_COUNT(h);
	parseBases(_bases, st, &h, options);

	otl_ClassnameHash *s, *tmp;
	HASH_ITER(hh, h, s, tmp) {
		HASH_DEL(h, s);
		sdsfree(s->className);
		FREE(s);
	}

	return (otl_Subtable *)st;
}

caryll_Buffer *otfcc_build_gpos_markToSingle(const otl_Subtable *_subtable, otl_BuildHeuristics heuristics) {
	const subtable_gpos_markToSingle *subtable = &(_subtable->gpos_markToSingle);
	otl_Coverage *marks = Coverage.create();
	for (glyphid_t j = 0; j < subtable->markArray.length; j++) {
		Coverage.push(marks, Handle.dup(subtable->markArray.items[j].glyph));
	}
	otl_Coverage *bases = Coverage.create();
	for (glyphid_t j = 0; j < subtable->baseArray.length; j++) {
		Coverage.push(bases, Handle.dup(subtable->baseArray.items[j].glyph));
	}

	bk_Block *root = bk_new_Block(b16, 1,                                            // format
	                              p16, bk_newBlockFromBuffer(Coverage.build(marks)), // markCoverage
	                              p16, bk_newBlockFromBuffer(Coverage.build(bases)), // baseCoverage
	                              b16, subtable->classCount,                         // classCont
	                              bkover);

	bk_Block *markArray = bk_new_Block(b16, subtable->markArray.length, // markCount
	                                   bkover);
	for (glyphid_t j = 0; j < subtable->markArray.length; j++) {
		bk_push(markArray,                                              // markArray item
		        b16, subtable->markArray.items[j].markClass,            // markClass
		        p16, bkFromAnchor(subtable->markArray.items[j].anchor), // Anchor
		        bkover);
	}

	bk_Block *baseArray = bk_new_Block(b16, subtable->baseArray.length, // baseCount
	                                   bkover);
	for (glyphid_t j = 0; j < subtable->baseArray.length; j++) {
		for (glyphclass_t k = 0; k < subtable->classCount; k++) {
			bk_push(baseArray, p16, bkFromAnchor(subtable->baseArray.items[j].anchors[k]), bkover);
		}
	}

	bk_push(root, p16, markArray, p16, baseArray, bkover);
	Coverage.free(marks);
	Coverage.free(bases);
	return bk_build_Block(root);
}
