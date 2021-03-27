#include "gpos-pair.h"
#include "gpos-common.h"

static INLINE void initGposPair(subtable_gpos_pair *subtable) {
	subtable->first = NULL;
	subtable->second = NULL;
	subtable->firstValues = NULL;
	subtable->secondValues = NULL;
}
static INLINE void disposeGposPair(subtable_gpos_pair *subtable) {
	if (subtable->firstValues) {
		for (glyphclass_t j = 0; j <= subtable->first->maxclass; j++) {
			FREE(subtable->firstValues[j]);
		}
		FREE(subtable->firstValues);
	}
	if (subtable->secondValues) {
		for (glyphclass_t j = 0; j <= subtable->first->maxclass; j++) {
			FREE(subtable->secondValues[j]);
		}
		FREE(subtable->secondValues);
	}
	DELETE(ClassDef.free, subtable->first);
	DELETE(ClassDef.free, subtable->second);
}

caryll_standardRefType(subtable_gpos_pair, iSubtable_gpos_pair, initGposPair, disposeGposPair);

typedef struct {
	int gid;
	int cid;
	UT_hash_handle hh;
} pair_classifier_hash;

otl_Subtable *otl_read_gpos_pair(const font_file_pointer data, uint32_t tableLength,
                                 uint32_t offset, const glyphid_t maxGlyphs,
                                 const otfcc_Options *options) {
	subtable_gpos_pair *subtable = iSubtable_gpos_pair.create();

	checkLength(offset + 2);
	uint16_t subtableFormat = read_16u(data + offset);
	if (subtableFormat == 1) {
		// pair adjustment by individuals
		otl_Coverage *cov = Coverage.read(data, tableLength, offset + read_16u(data + offset + 2));
		NEW(subtable->first);
		subtable->first->numGlyphs = cov->numGlyphs;
		subtable->first->maxclass = cov->numGlyphs - 1;
		subtable->first->glyphs = cov->glyphs;
		NEW(subtable->first->classes, cov->numGlyphs);
		for (glyphid_t j = 0; j < cov->numGlyphs; j++) {
			subtable->first->classes[j] = j;
		}
		FREE(cov);

		uint16_t format1 = read_16u(data + offset + 4);
		uint16_t format2 = read_16u(data + offset + 6);
		uint8_t len1 = position_format_length(format1);
		uint8_t len2 = position_format_length(format2);

		glyphid_t pairSetCount = read_16u(data + offset + 8);
		if (pairSetCount != subtable->first->numGlyphs) goto FAIL;
		checkLength(offset + 10 + 2 * pairSetCount);

		// pass 1: check length
		for (glyphid_t j = 0; j < pairSetCount; j++) {
			uint32_t pairSetOffset = offset + read_16u(data + offset + 10 + 2 * j);
			checkLength(pairSetOffset + 2);
			glyphid_t pairCount = read_16u(data + pairSetOffset);
			checkLength(pairSetOffset + 2 + (2 + len1 + len2) * pairCount);
		}
		// pass 2: make hashtable about second glyphs to construct a classdec
		pair_classifier_hash *h = NULL;
		for (glyphid_t j = 0; j < pairSetCount; j++) {
			uint32_t pairSetOffset = offset + read_16u(data + offset + 10 + 2 * j);
			glyphid_t pairCount = read_16u(data + pairSetOffset);
			for (glyphid_t k = 0; k < pairCount; k++) {
				int second = read_16u(data + pairSetOffset + 2 + (2 + len1 + len2) * k);
				pair_classifier_hash *s;
				HASH_FIND_INT(h, &second, s);
				if (!s) {
					NEW(s);
					s->gid = second;
					s->cid = HASH_COUNT(h) + 1;
					HASH_ADD_INT(h, gid, s);
				}
			}
		}
		// pass 3: make the matrix
		NEW(subtable->second);
		subtable->second->numGlyphs = HASH_COUNT(h);
		subtable->second->maxclass = HASH_COUNT(h);
		NEW(subtable->second->classes, subtable->second->numGlyphs);
		NEW(subtable->second->glyphs, subtable->second->numGlyphs);
		glyphclass_t class2Count = subtable->second->maxclass + 1;

		NEW(subtable->firstValues, subtable->first->maxclass + 1);
		NEW(subtable->secondValues, subtable->first->maxclass + 1);
		for (glyphclass_t j = 0; j <= subtable->first->maxclass; j++) {
			NEW(subtable->firstValues[j], class2Count);
			NEW(subtable->secondValues[j], class2Count);
			for (glyphclass_t k = 0; k < class2Count; k++) {
				subtable->firstValues[j][k] = position_zero();
				subtable->secondValues[j][k] = position_zero();
			}
		};
		for (glyphclass_t j = 0; j <= subtable->first->maxclass; j++) {
			uint32_t pairSetOffset = offset + read_16u(data + offset + 10 + 2 * j);
			glyphid_t pairCount = read_16u(data + pairSetOffset);
			for (glyphid_t k = 0; k < pairCount; k++) {
				int second = read_16u(data + pairSetOffset + 2 + (2 + len1 + len2) * k);
				pair_classifier_hash *s;
				HASH_FIND_INT(h, &second, s);
				if (s) {
					subtable->firstValues[j][s->cid] = read_gpos_value(
					    data, tableLength, pairSetOffset + 2 + (2 + len1 + len2) * k + 2, format1);
					subtable->secondValues[j][s->cid] = read_gpos_value(
					    data, tableLength, pairSetOffset + 2 + (2 + len1 + len2) * k + 2 + len1,
					    format2);
				}
			}
		}
		// pass 4: return
		pair_classifier_hash *s, *tmp;
		glyphid_t jj = 0;
		HASH_ITER(hh, h, s, tmp) {
			subtable->second->glyphs[jj] = Handle.fromIndex(s->gid);
			subtable->second->classes[jj] = s->cid;
			jj++;
			HASH_DEL(h, s);
			FREE(s);
		}
		return (otl_Subtable *)subtable;
	} else if (subtableFormat == 2) {
		// pair adjustment by classes
		checkLength(offset + 16);
		uint16_t format1 = read_16u(data + offset + 4);
		uint16_t format2 = read_16u(data + offset + 6);
		uint8_t len1 = position_format_length(format1);
		uint8_t len2 = position_format_length(format2);
		otl_Coverage *cov = Coverage.read(data, tableLength, offset + read_16u(data + offset + 2));
		subtable->first = ClassDef.read(data, tableLength, offset + read_16u(data + offset + 8));
		subtable->first = ClassDef.expand(cov, subtable->first);
		DELETE(Coverage.free, cov);
		subtable->second = ClassDef.read(data, tableLength, offset + read_16u(data + offset + 10));
		if (!subtable->first || !subtable->second) goto FAIL;
		glyphclass_t class1Count = read_16u(data + offset + 12);
		glyphclass_t class2Count = read_16u(data + offset + 14);
		if (subtable->first->maxclass + 1 != class1Count) goto FAIL;
		if (subtable->second->maxclass + 1 != class2Count) goto FAIL;

		checkLength(offset + 16 + class1Count * class2Count * (len1 + len2));
		// read the matrix
		NEW(subtable->firstValues, class1Count);
		NEW(subtable->secondValues, class1Count);

		for (glyphclass_t j = 0; j < class1Count; j++) {
			NEW(subtable->firstValues[j], class2Count);
			NEW(subtable->secondValues[j], class2Count);
			for (glyphclass_t k = 0; k < class2Count; k++) {
				subtable->firstValues[j][k] =
				    read_gpos_value(data, tableLength,
				                    offset + 16 + (j * class2Count + k) * (len1 + len2), format1);
				subtable->secondValues[j][k] = read_gpos_value(
				    data, tableLength, offset + 16 + (j * class2Count + k) * (len1 + len2) + len1,
				    format2);
			}
		}

		return (otl_Subtable *)subtable;
	}
FAIL:
	iSubtable_gpos_pair.free(subtable);
	return NULL;
}

json_value *otl_gpos_dump_pair(const otl_Subtable *_subtable) {
	const subtable_gpos_pair *subtable = &(_subtable->gpos_pair);
	json_value *st = json_object_new(3);
	json_object_push(st, "first", ClassDef.dump(subtable->first));
	json_object_push(st, "second", ClassDef.dump(subtable->second));

	json_value *mat = json_array_new(subtable->first->maxclass + 1);
	for (glyphclass_t j = 0; j <= subtable->first->maxclass; j++) {
		json_value *row = json_array_new(subtable->second->maxclass + 1);
		for (glyphclass_t k = 0; k <= subtable->second->maxclass; k++) {
			uint8_t f1 = required_position_format(subtable->firstValues[j][k]);
			uint8_t f2 = required_position_format(subtable->secondValues[j][k]);
			if (f1 | f2) {
				if (f1 == FORMAT_DWIDTH && f2 == 0) {
					// simple kerning
					json_array_push(row, json_new_position(subtable->firstValues[j][k].dWidth));
				} else {
					json_value *pair = json_object_new(2);
					if (f1) {
						json_object_push(pair, "first",
						                 gpos_dump_value(subtable->firstValues[j][k]));
					}
					if (f2) {
						json_object_push(pair, "second",
						                 gpos_dump_value(subtable->secondValues[j][k]));
					}
					json_array_push(row, pair);
				}
			} else {
				json_array_push(row, json_new_position(0));
			}
		}
		json_array_push(mat, preserialize(row));
	}
	json_object_push(st, "matrix", mat);
	return st;
}
otl_Subtable *otl_gpos_parse_pair(const json_value *_subtable, const otfcc_Options *options) {
	subtable_gpos_pair *subtable = iSubtable_gpos_pair.create();

	json_value *_mat = json_obj_get_type(_subtable, "matrix", json_array);
	subtable->first = ClassDef.parse(json_obj_get_type(_subtable, "first", json_object));
	subtable->second = ClassDef.parse(json_obj_get_type(_subtable, "second", json_object));
	if (!_mat || !subtable->first || !subtable->second) goto FAIL;

	glyphclass_t class1Count = subtable->first->maxclass + 1;
	glyphclass_t class2Count = subtable->second->maxclass + 1;

	NEW(subtable->firstValues, class1Count);
	NEW(subtable->secondValues, class1Count);

	for (glyphclass_t j = 0; j < class1Count; j++) {
		NEW(subtable->firstValues[j], class2Count);
		NEW(subtable->secondValues[j], class2Count);
		for (glyphclass_t k = 0; k < class2Count; k++) {
			subtable->firstValues[j][k] = position_zero();
			subtable->secondValues[j][k] = position_zero();
		}
	}
	for (glyphclass_t j = 0; j < class1Count && j < _mat->u.array.length; j++) {
		json_value *_row = _mat->u.array.values[j];
		if (!_row || _row->type != json_array) continue;
		for (glyphclass_t k = 0; k < class2Count && k < _row->u.array.length; k++) {
			json_value *_item = _row->u.array.values[k];
			if (_item->type == json_integer) {
				subtable->firstValues[j][k].dWidth = _item->u.integer;
			} else if (_item->type == json_double) {
				subtable->firstValues[j][k].dWidth = _item->u.dbl;
			} else if (_item->type == json_object) {
				subtable->firstValues[j][k] = gpos_parse_value(json_obj_get(_item, "first"));
				subtable->secondValues[j][k] = gpos_parse_value(json_obj_get(_item, "second"));
			}
		}
	}
	return (otl_Subtable *)subtable;
FAIL:
	iSubtable_gpos_pair.free(subtable);
	return NULL;
}
static otl_Coverage *covFromCD(otl_ClassDef *cd) {
	otl_Coverage *cov;
	NEW(cov);
	cov->numGlyphs = cd->numGlyphs;
	NEW(cov->glyphs, cd->numGlyphs);
	for (glyphid_t j = 0; j < cd->numGlyphs; j++) {
		cov->glyphs[j] = Handle.dup(cd->glyphs[j]);
	}
	return cov;
}

typedef struct {
	glyphid_t gid;
	otl_PositionValue *fv;
	otl_PositionValue *sv;
} IndividualGposPair;

static int by_pairSecondGlyph(const void *a, const void *b) {
	return ((IndividualGposPair *)a)->gid - ((IndividualGposPair *)b)->gid;
}

bk_Block *otfcc_build_gpos_pair_individual(const otl_Subtable *_subtable) {
	const subtable_gpos_pair *subtable = &(_subtable->gpos_pair);
	uint16_t format1 = 0;
	uint16_t format2 = 0;
	glyphclass_t class1Count = subtable->first->maxclass + 1;
	glyphclass_t class2Count = subtable->second->maxclass + 1;

	for (glyphclass_t j = 0; j < class1Count; j++) {
		for (glyphclass_t k = 0; k < class2Count; k++) {
			format1 |= required_position_format(subtable->firstValues[j][k]);
			format2 |= required_position_format(subtable->secondValues[j][k]);
		}
	}
	glyphid_t *pairCounts;
	NEW(pairCounts, subtable->first->numGlyphs);
	for (glyphid_t j = 0; j < subtable->first->numGlyphs; j++) {
		pairCounts[j] = 0;
		for (glyphid_t k = 0; k < subtable->second->numGlyphs; k++) {
			glyphclass_t c1 = subtable->first->classes[j];
			glyphclass_t c2 = subtable->second->classes[k];
			if (required_position_format(subtable->firstValues[c1][c2]) |
			    required_position_format(subtable->secondValues[c1][c2])) {
				pairCounts[j] += 1;
			}
		}
	}
	otl_Coverage *cov = covFromCD(subtable->first);
	Coverage.shrink(cov, true);
	bk_Block *root = bk_new_Block(b16, 1,                                          // PosFormat
	                              p16, bk_newBlockFromBuffer(Coverage.build(cov)), // Coverage
	                              b16, format1,                                    // ValueFormat1
	                              b16, format2,                                    // ValueFormat2
	                              b16, subtable->first->numGlyphs,                 // PairSetCount
	                              bkover);

	for (glyphid_t j = 0; j < cov->numGlyphs; j++) {
		tableid_t currentPairCount = 0;
		glyphclass_t c1 = 0;
		for (glyphid_t k = 0; k < subtable->first->numGlyphs; k++) {
			if (subtable->first->glyphs[k].index == cov->glyphs[j].index) {
				// The coverage is sorted, not direct correspondence with subtable->first.
				c1 = subtable->first->classes[k];
				currentPairCount = pairCounts[k];
			}
		}
		bk_Block *pairSet = bk_new_Block(b16, currentPairCount, // PairValueCount
		                                 bkover);
		IndividualGposPair *pairs;
		NEW_CLEAN_N(pairs, currentPairCount);
		size_t n = 0;
		for (glyphid_t k = 0; k < subtable->second->numGlyphs; k++) {
			glyphclass_t c2 = subtable->second->classes[k];
			if (required_position_format(subtable->firstValues[c1][c2]) |
			    required_position_format(subtable->secondValues[c1][c2])) {
				pairs[n].gid = subtable->second->glyphs[k].index;
				pairs[n].fv = &subtable->firstValues[c1][c2];
				pairs[n].sv = &subtable->secondValues[c1][c2];
				n++;
			}
		}
		qsort(pairs, currentPairCount, sizeof(IndividualGposPair), by_pairSecondGlyph);
		for (size_t n = 0; n < currentPairCount; n++) {
			bk_push(pairSet, b16, pairs[n].gid,                      // SecondGlyph
			        bkembed, bk_gpos_value(*(pairs[n].fv), format1), // Value1
			        bkembed, bk_gpos_value(*(pairs[n].sv), format2), // Value2
			        bkover);
		}
		FREE(pairs);
		bk_push(root, p16, pairSet, bkover);
	}
	DELETE(Coverage.free, cov);
	FREE(pairCounts);
	return root;
}
bk_Block *otfcc_build_gpos_pair_classes(const otl_Subtable *_subtable) {
	const subtable_gpos_pair *subtable = &(_subtable->gpos_pair);
	uint16_t format1 = 0;
	uint16_t format2 = 0;
	glyphclass_t class1Count = subtable->first->maxclass + 1;
	glyphclass_t class2Count = subtable->second->maxclass + 1;

	for (glyphclass_t j = 0; j < class1Count; j++) {
		for (glyphclass_t k = 0; k < class2Count; k++) {
			format1 |= required_position_format(subtable->firstValues[j][k]);
			format2 |= required_position_format(subtable->secondValues[j][k]);
		}
	}
	otl_Coverage *cov = covFromCD(subtable->first);
	bk_Block *root =
	    bk_new_Block(b16, 2,                                                       // PosFormat
	                 p16, bk_newBlockFromBuffer(Coverage.build(cov)),              // Coverage
	                 b16, format1,                                                 // ValueFormat1
	                 b16, format2,                                                 // ValueFormat2
	                 p16, bk_newBlockFromBuffer(ClassDef.build(subtable->first)),  // ClassDef1
	                 p16, bk_newBlockFromBuffer(ClassDef.build(subtable->second)), // ClassDef2
	                 b16, class1Count,                                             // Class1Count
	                 b16, class2Count,                                             // Class2Count
	                 bkover);
	for (glyphclass_t j = 0; j < class1Count; j++) {
		for (glyphclass_t k = 0; k < class2Count; k++) {
			bk_push(root, bkembed, bk_gpos_value(subtable->firstValues[j][k], format1), // Value1
			        bkembed, bk_gpos_value(subtable->secondValues[j][k], format2),      // Value2
			        bkover);
		}
	}
	DELETE(Coverage.free, cov);
	return root;
}
caryll_Buffer *otfcc_build_gpos_pair(const otl_Subtable *_subtable, otl_BuildHeuristics heuristics) {
	bk_Block *format1 = otfcc_build_gpos_pair_individual(_subtable);
	bk_Block *format2 = otfcc_build_gpos_pair_classes(_subtable);
	bk_Graph *g1 = bk_newGraphFromRootBlock(format1);
	bk_Graph *g2 = bk_newGraphFromRootBlock(format2);
	bk_minimizeGraph(g1);
	bk_minimizeGraph(g2);
	if (bk_estimateSizeOfGraph(g1) > bk_estimateSizeOfGraph(g2)) {
		// Choose pair adjustment by classes
		bk_delete_Graph(g1);
		bk_untangleGraph(g2);
		caryll_Buffer *buf = bk_build_Graph(g2);
		bk_delete_Graph(g2);
		return buf;
	} else {
		// Choose pair adjustment by individuals
		bk_delete_Graph(g2);
		bk_untangleGraph(g1);
		caryll_Buffer *buf = bk_build_Graph(g1);
		bk_delete_Graph(g1);
		return buf;
	}
}
