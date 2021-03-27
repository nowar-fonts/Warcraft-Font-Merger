#include "gpos-mark-to-ligature.h"
#include "gpos-common.h"

static void deleteLigArrayItem(otl_LigatureBaseRecord *entry) {
	Handle.dispose(&entry->glyph);
	if (entry->anchors) {
		for (glyphid_t k = 0; k < entry->componentCount; k++)
			FREE(entry->anchors[k]);
		FREE(entry->anchors);
	}
}
static caryll_ElementInterface(otl_LigatureBaseRecord) la_typeinfo = {
    .init = NULL, .copy = NULL, .dispose = deleteLigArrayItem};

caryll_standardVectorImpl(otl_LigatureArray, otl_LigatureBaseRecord, la_typeinfo,
                          otl_iLigatureArray);

static INLINE void initMarkToLigature(subtable_gpos_markToLigature *subtable) {
	otl_iMarkArray.init(&subtable->markArray);
	otl_iLigatureArray.init(&subtable->ligArray);
}
static INLINE void disposeMarkToLigature(subtable_gpos_markToLigature *subtable) {
	otl_iMarkArray.dispose(&subtable->markArray);
	otl_iLigatureArray.dispose(&subtable->ligArray);
}

caryll_standardRefType(subtable_gpos_markToLigature, iSubtable_gpos_markToLigature,
                       initMarkToLigature, disposeMarkToLigature);

otl_Subtable *otl_read_gpos_markToLigature(const font_file_pointer data, uint32_t tableLength,
                                           uint32_t offset, const glyphid_t maxGlyphs,
                                           const otfcc_Options *options) {
	subtable_gpos_markToLigature *subtable = iSubtable_gpos_markToLigature.create();
	otl_Coverage *marks = NULL;
	otl_Coverage *bases = NULL;
	if (tableLength < offset + 12) goto FAIL;

	marks = Coverage.read(data, tableLength, offset + read_16u(data + offset + 2));
	bases = Coverage.read(data, tableLength, offset + read_16u(data + offset + 4));
	if (!marks || marks->numGlyphs == 0 || !bases || bases->numGlyphs == 0) goto FAIL;
	subtable->classCount = read_16u(data + offset + 6);

	uint32_t markArrayOffset = offset + read_16u(data + offset + 8);
	otl_readMarkArray(&subtable->markArray, marks, data, tableLength, markArrayOffset);

	uint32_t ligArrayOffset = offset + read_16u(data + offset + 10);
	checkLength(ligArrayOffset + 2 + 2 * bases->numGlyphs);
	if (read_16u(data + ligArrayOffset) != bases->numGlyphs) goto FAIL;

	for (glyphid_t j = 0; j < bases->numGlyphs; j++) {
		otl_LigatureBaseRecord lig;
		lig.glyph = Handle.dup(bases->glyphs[j]);
		uint32_t ligAttachOffset = ligArrayOffset + read_16u(data + ligArrayOffset + 2 + j * 2);
		checkLength(ligAttachOffset + 2);
		lig.componentCount = read_16u(data + ligAttachOffset);

		checkLength(ligAttachOffset + 2 + 2 * lig.componentCount * subtable->classCount);
		NEW(lig.anchors, lig.componentCount);

		uint32_t _offset = ligAttachOffset + 2;
		for (glyphid_t k = 0; k < lig.componentCount; k++) {
			NEW(lig.anchors[k], subtable->classCount);
			for (glyphclass_t m = 0; m < subtable->classCount; m++) {
				uint32_t anchorOffset = read_16u(data + _offset);
				if (anchorOffset) {
					lig.anchors[k][m] =
					    otl_read_anchor(data, tableLength, ligAttachOffset + anchorOffset);
				} else {
					lig.anchors[k][m] = otl_anchor_absent();
				}
				_offset += 2;
			}
		}
		otl_iLigatureArray.push(&subtable->ligArray, lig);
	}
	if (marks) Coverage.free(marks);
	if (bases) Coverage.free(bases);
	return (otl_Subtable *)subtable;
FAIL:
	if (marks) Coverage.free(marks);
	if (bases) Coverage.free(bases);
	iSubtable_gpos_markToLigature.free(subtable);
	return NULL;
}

json_value *otl_gpos_dump_markToLigature(const otl_Subtable *st) {
	const subtable_gpos_markToLigature *subtable = &(st->gpos_markToLigature);
	json_value *_subtable = json_object_new(3);
	json_value *_marks = json_object_new(subtable->markArray.length);
	json_value *_bases = json_object_new(subtable->ligArray.length);
	for (glyphid_t j = 0; j < subtable->markArray.length; j++) {
		json_value *_mark = json_object_new(3);
		sds markClassName = sdscatfmt(sdsempty(), "ac_%i", subtable->markArray.items[j].markClass);
		json_object_push(_mark, "class",
		                 json_string_new_length((uint32_t)sdslen(markClassName), markClassName));
		sdsfree(markClassName);
		json_object_push(_mark, "x", json_integer_new(subtable->markArray.items[j].anchor.x));
		json_object_push(_mark, "y", json_integer_new(subtable->markArray.items[j].anchor.y));
		json_object_push(_marks, subtable->markArray.items[j].glyph.name, preserialize(_mark));
	}
	for (glyphid_t j = 0; j < subtable->ligArray.length; j++) {
		otl_LigatureBaseRecord *base = &subtable->ligArray.items[j];
		json_value *_base = json_array_new(base->componentCount);
		for (glyphid_t k = 0; k < base->componentCount; k++) {
			json_value *_bk = json_object_new(subtable->classCount);
			for (glyphclass_t m = 0; m < subtable->classCount; m++) {
				if (base->anchors[k][m].present) {
					json_value *_anchor = json_object_new(2);
					json_object_push(_anchor, "x", json_integer_new(base->anchors[k][m].x));
					json_object_push(_anchor, "y", json_integer_new(base->anchors[k][m].y));
					sds markClassName = sdscatfmt(sdsempty(), "ac_%i", m);
					json_object_push_length(_bk, (uint32_t)sdslen(markClassName), markClassName,
					                        _anchor);
					sdsfree(markClassName);
				}
			}
			json_array_push(_base, _bk);
		}
		json_object_push(_bases, base->glyph.name, preserialize(_base));
	}
	json_object_push(_subtable, "classCount", json_integer_new(subtable->classCount));
	json_object_push(_subtable, "marks", _marks);
	json_object_push(_subtable, "bases", _bases);
	return _subtable;
}

static void parseBases(json_value *_bases, subtable_gpos_markToLigature *subtable,
                       otl_ClassnameHash **h, const otfcc_Options *options) {
	glyphclass_t classCount = HASH_COUNT(*h);

	for (glyphid_t j = 0; j < _bases->u.object.length; j++) {
		char *gname = _bases->u.object.values[j].name;
		otl_LigatureBaseRecord lig;
		lig.componentCount = 0;
		lig.anchors = NULL;
		lig.glyph = Handle.fromName(
		    sdsnewlen(_bases->u.object.values[j].name, _bases->u.object.values[j].name_length));

		json_value *baseRecord = _bases->u.object.values[j].value;
		if (!baseRecord || baseRecord->type != json_array) {
			otl_iLigatureArray.push(&subtable->ligArray, lig);
			continue;
		}
		lig.componentCount = baseRecord->u.array.length;

		NEW(lig.anchors, lig.componentCount);

		for (glyphid_t k = 0; k < lig.componentCount; k++) {
			json_value *_componentRecord = baseRecord->u.array.values[k];
			NEW(lig.anchors[k], classCount);
			for (glyphclass_t m = 0; m < classCount; m++) {
				lig.anchors[k][m] = otl_anchor_absent();
			}
			if (!_componentRecord || _componentRecord->type != json_object) { continue; }
			for (glyphclass_t m = 0; m < _componentRecord->u.object.length; m++) {
				sds className = sdsnewlen(_componentRecord->u.object.values[m].name,
				                          _componentRecord->u.object.values[m].name_length);
				otl_ClassnameHash *s;
				HASH_FIND_STR(*h, className, s);
				if (!s) {
					logWarning("[OTFCC-fea] Invalid anchor class name <%s> for /%s. This base "
					           "anchor is ignored.\n",
					           className, gname);
					goto NEXT;
				}
				lig.anchors[k][s->classID] =
				    otl_parse_anchor(_componentRecord->u.object.values[m].value);

			NEXT:
				sdsfree(className);
			}
		}
		otl_iLigatureArray.push(&subtable->ligArray, lig);
	}
}
otl_Subtable *otl_gpos_parse_markToLigature(const json_value *_subtable,
                                            const otfcc_Options *options) {
	json_value *_marks = json_obj_get_type(_subtable, "marks", json_object);
	json_value *_bases = json_obj_get_type(_subtable, "bases", json_object);
	if (!_marks || !_bases) return NULL;
	subtable_gpos_markToLigature *st = iSubtable_gpos_markToLigature.create();
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

caryll_Buffer *otfcc_build_gpos_markToLigature(const otl_Subtable *_subtable, otl_BuildHeuristics heuristics) {
	const subtable_gpos_markToLigature *subtable = &(_subtable->gpos_markToLigature);
	otl_Coverage *marks = Coverage.create();
	for (glyphid_t j = 0; j < subtable->markArray.length; j++) {
		Coverage.push(marks, Handle.dup(subtable->markArray.items[j].glyph));
	}
	otl_Coverage *bases = Coverage.create();
	for (glyphid_t j = 0; j < subtable->ligArray.length; j++) {
		Coverage.push(bases, Handle.dup(subtable->ligArray.items[j].glyph));
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

	bk_Block *ligatureArray = bk_new_Block(b16, subtable->ligArray.length, bkover);
	for (glyphid_t j = 0; j < subtable->ligArray.length; j++) {
		bk_Block *attach =
		    bk_new_Block(b16, subtable->ligArray.items[j].componentCount, // componentCount
		                 bkover);
		for (glyphid_t k = 0; k < subtable->ligArray.items[j].componentCount; k++) {
			for (glyphclass_t m = 0; m < subtable->classCount; m++) {
				bk_push(attach, p16, bkFromAnchor(subtable->ligArray.items[j].anchors[k][m]),
				        bkover);
			}
		}
		bk_push(ligatureArray, p16, attach, bkover);
	}

	bk_push(root, p16, markArray, p16, ligatureArray, bkover);
	Coverage.free(marks);
	Coverage.free(bases);
	return bk_build_Block(root);
}
