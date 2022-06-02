#include "BASE.h"

#include "otl/private.h"

static void deleteBaseAxis(MOVE otl_BaseAxis *axis) {
	if (!axis) return;
	if (axis->entries) {
		for (tableid_t j = 0; j < axis->scriptCount; j++) {
			if (axis->entries[j].baseValues) FREE(axis->entries[j].baseValues);
		}
		FREE(axis->entries);
	}
}

static INLINE void disposeBASE(MOVE table_BASE *base) {
	deleteBaseAxis(base->horizontal);
	deleteBaseAxis(base->vertical);
}

caryll_standardRefType(table_BASE, table_iBASE, disposeBASE);

static int16_t readBaseValue(font_file_pointer data, uint32_t tableLength, uint16_t offset) {
	checkLength(offset + 4);
	return read_16s(data + offset + 2);
FAIL:
	return 0;
}

static void readBaseScript(const font_file_pointer data, uint32_t tableLength, uint16_t offset,
                           otl_BaseScriptEntry *entry, uint32_t *baseTagList, uint16_t nBaseTags) {
	entry->baseValuesCount = 0;
	entry->baseValues = NULL;
	entry->defaultBaselineTag = 0;
	checkLength(offset + 2); // care about base values only now
	uint16_t baseValuesOffset = read_16u(data + offset);
	if (baseValuesOffset) {
		baseValuesOffset += offset;
		checkLength(baseValuesOffset + 4);
		uint16_t defaultIndex = read_16u(data + baseValuesOffset) % nBaseTags;
		entry->defaultBaselineTag = baseTagList[defaultIndex];
		entry->baseValuesCount = read_16u(data + baseValuesOffset + 2);
		if (entry->baseValuesCount != nBaseTags) goto FAIL;
		checkLength(baseValuesOffset + 4 + 2 * entry->baseValuesCount);
		NEW(entry->baseValues, entry->baseValuesCount);
		for (tableid_t j = 0; j < entry->baseValuesCount; j++) {
			entry->baseValues[j].tag = baseTagList[j];
			uint16_t _valOffset = read_16u(data + baseValuesOffset + 4 + 2 * j);
			if (_valOffset) {
				entry->baseValues[j].coordinate =
				    readBaseValue(data, tableLength, baseValuesOffset + _valOffset);
			} else {
				entry->baseValues[j].coordinate = 0;
			}
		}
		return;
	}
FAIL:
	entry->baseValuesCount = 0;
	if (entry->baseValues) FREE(entry->baseValues);
	entry->baseValues = NULL;
	entry->defaultBaselineTag = 0;
	return;
}

static otl_BaseAxis *readAxis(font_file_pointer data, uint32_t tableLength, uint16_t offset) {
	otl_BaseAxis *axis = NULL;
	uint32_t *baseTagList = NULL;
	checkLength(offset + 4);

	// Read BaseTagList
	uint16_t baseTagListOffset = offset + read_16u(data + offset);
	if (baseTagListOffset <= offset) goto FAIL;
	checkLength(baseTagListOffset + 2);
	uint16_t nBaseTags = read_16u(data + baseTagListOffset);
	if (!nBaseTags) goto FAIL;
	checkLength(baseTagListOffset + 2 + 4 * nBaseTags);
	NEW(baseTagList, nBaseTags);
	for (uint16_t j = 0; j < nBaseTags; j++) {
		baseTagList[j] = read_32u(data + baseTagListOffset + 2 + j * 4);
	}

	uint16_t baseScriptListOffset = offset + read_16u(data + offset + 2);
	if (baseScriptListOffset <= offset) goto FAIL;
	checkLength(baseScriptListOffset + 2);
	tableid_t nBaseScripts = read_16u(data + baseScriptListOffset);
	checkLength(baseScriptListOffset + 2 + 6 * nBaseScripts);
	NEW(axis);
	axis->scriptCount = nBaseScripts;
	NEW(axis->entries, nBaseScripts);
	for (tableid_t j = 0; j < nBaseScripts; j++) {
		axis->entries[j].tag = read_32u(data + baseScriptListOffset + 2 + 6 * j);
		uint16_t baseScriptOffset = read_16u(data + baseScriptListOffset + 2 + 6 * j + 4);
		if (baseScriptOffset) {
			readBaseScript(data, tableLength, baseScriptListOffset + baseScriptOffset,
			               &(axis->entries[j]), baseTagList, nBaseTags);
		} else {
			axis->entries[j].baseValuesCount = 0;
			axis->entries[j].baseValues = NULL;
			axis->entries[j].defaultBaselineTag = 0;
		}
	}
	return axis;

FAIL:
	if (baseTagList) FREE(baseTagList);
	DELETE(deleteBaseAxis, axis);
	return axis;
}

table_BASE *otfcc_readBASE(const otfcc_Packet packet, const otfcc_Options *options) {
	table_BASE *base = NULL;
	FOR_TABLE('BASE', table) {
		font_file_pointer data = table.data;
		uint32_t tableLength = table.length;
		checkLength(8);
		NEW(base);
		uint16_t offsetH = read_16u(data + 4);
		if (offsetH) base->horizontal = readAxis(data, tableLength, offsetH);
		uint16_t offsetV = read_16u(data + 6);
		if (offsetV) base->vertical = readAxis(data, tableLength, offsetV);
		return base;
	FAIL:
		logWarning("Table 'BASE' Corrupted");
		DELETE(table_iBASE.free, base);
	}
	return base;
}

static json_value *axisToJson(const otl_BaseAxis *axis) {
	json_value *_axis = json_object_new(axis->scriptCount);
	for (tableid_t j = 0; j < axis->scriptCount; j++) {
		if (!axis->entries[j].tag) continue;
		json_value *_entry = json_object_new(3);
		if (axis->entries[j].defaultBaselineTag) {
			char tag[4];
			tag2str(axis->entries[j].defaultBaselineTag, tag);
			json_object_push(_entry, "defaultBaseline", json_string_new_length(4, tag));
		}
		json_value *_values = json_object_new(axis->entries[j].baseValuesCount);
		for (tableid_t k = 0; k < axis->entries[j].baseValuesCount; k++) {
			if (axis->entries[j].baseValues[k].tag) {

				json_object_push_tag(_values, axis->entries[j].baseValues[k].tag,
				                     json_new_position(axis->entries[j].baseValues[k].coordinate));
			}
		}
		json_object_push(_entry, "baselines", _values);
		json_object_push_tag(_axis, axis->entries[j].tag, _entry);
	}
	return _axis;
}

void otfcc_dumpBASE(const table_BASE *base, json_value *root, const otfcc_Options *options) {
	if (!base) return;
	loggedStep("BASE") {
		json_value *_base = json_object_new(2);
		if (base->horizontal) json_object_push(_base, "horizontal", axisToJson(base->horizontal));
		if (base->vertical) json_object_push(_base, "vertical", axisToJson(base->vertical));
		json_object_push(root, "BASE", _base);
	}
}

static void baseScriptFromJson(const json_value *_sr, otl_BaseScriptEntry *entry) {
	entry->defaultBaselineTag = str2tag(json_obj_getstr_share(_sr, "defaultBaseline"));
	json_value *_basevalues = json_obj_get_type(_sr, "baselines", json_object);
	if (!_basevalues) {
		entry->baseValuesCount = 0;
		entry->baseValues = NULL;
	} else {
		entry->baseValuesCount = _basevalues->u.object.length;
		NEW(entry->baseValues, entry->baseValuesCount);
		for (tableid_t j = 0; j < entry->baseValuesCount; j++) {
			entry->baseValues[j].tag = str2tag(_basevalues->u.object.values[j].name);
			entry->baseValues[j].coordinate = json_numof(_basevalues->u.object.values[j].value);
		}
	}
}

static int by_script_tag(const void *a, const void *b) {
	return ((otl_BaseScriptEntry *)a)->tag - ((otl_BaseScriptEntry *)b)->tag;
}

static otl_BaseAxis *axisFromJson(const json_value *_axis) {
	if (!_axis) return NULL;
	otl_BaseAxis *axis;
	NEW(axis);
	axis->scriptCount = _axis->u.object.length;
	NEW(axis->entries, axis->scriptCount);
	tableid_t jj = 0;
	for (tableid_t j = 0; j < axis->scriptCount; j++) {
		if (_axis->u.object.values[j].value &&
		    _axis->u.object.values[j].value->type == json_object) {
			axis->entries[jj].tag = str2tag(_axis->u.object.values[j].name);
			baseScriptFromJson(_axis->u.object.values[j].value, &(axis->entries[jj]));
			jj++;
		}
	}
	axis->scriptCount = jj;
	qsort(axis->entries, axis->scriptCount, sizeof(otl_BaseScriptEntry), by_script_tag);
	return axis;
}

table_BASE *otfcc_parseBASE(const json_value *root, const otfcc_Options *options) {
	table_BASE *base = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "BASE", json_object))) {
		loggedStep("BASE") {
			NEW(base);
			base->horizontal = axisFromJson(json_obj_get_type(table, "horizontal", json_object));
			base->vertical = axisFromJson(json_obj_get_type(table, "vertical", json_object));
		}
	}
	return base;
}

static int by_tag(const void *a, const void *b) {
	return *((uint32_t *)a) - *((uint32_t *)b);
}

bk_Block *axisToBk(const otl_BaseAxis *axis) {
	if (!axis) return NULL;
	struct {
		tableid_t size;
		uint32_t *items;
	} taglist;
	taglist.size = 0;
	taglist.items = NULL;

	for (tableid_t j = 0; j < axis->scriptCount; j++) {
		otl_BaseScriptEntry *entry = &(axis->entries[j]);
		if (entry->defaultBaselineTag) {
			bool found = false;
			for (tableid_t jk = 0; jk < taglist.size; jk++) {
				if (taglist.items[jk] == entry->defaultBaselineTag) {
					found = true;
					break;
				}
			}
			if (!found) {
				taglist.size += 1;
				RESIZE(taglist.items, taglist.size);
				taglist.items[taglist.size - 1] = entry->defaultBaselineTag;
			}
		}
		for (tableid_t k = 0; k < entry->baseValuesCount; k++) {
			uint32_t tag = entry->baseValues[k].tag;
			bool found = false;
			for (tableid_t jk = 0; jk < taglist.size; jk++) {
				if (taglist.items[jk] == tag) {
					found = true;
					break;
				}
			}
			if (!found) {
				taglist.size += 1;
				RESIZE(taglist.items, taglist.size);
				taglist.items[taglist.size - 1] = tag;
			}
		}
	}
	qsort(taglist.items, taglist.size, sizeof(uint32_t), by_tag);
	bk_Block *baseTagList = bk_new_Block(b16, taglist.size, bkover);
	for (tableid_t j = 0; j < taglist.size; j++) {
		bk_push(baseTagList, b32, taglist.items[j], bkover);
	}

	bk_Block *baseScriptList = bk_new_Block(b16, axis->scriptCount, bkover);
	for (tableid_t j = 0; j < axis->scriptCount; j++) {
		otl_BaseScriptEntry *entry = &(axis->entries[j]);
		bk_Block *baseValues = bk_new_Block(bkover);
		{
			tableid_t defaultIndex = 0;
			for (tableid_t m = 0; m < taglist.size; m++) {
				if (taglist.items[m] == entry->defaultBaselineTag) {
					defaultIndex = m;
					break;
				}
			}
			bk_push(baseValues, b16, defaultIndex, bkover);
		}
		bk_push(baseValues, b16, taglist.size, bkover);
		for (size_t m = 0; m < taglist.size; m++) {
			bool found = false;
			tableid_t foundIndex = 0;
			for (tableid_t k = 0; k < entry->baseValuesCount; k++) {
				if (entry->baseValues[k].tag == taglist.items[m]) {
					found = true, foundIndex = k;
					break;
				}
			}
			if (found) {
				bk_push(baseValues, // base value
				        p16,
				        bk_new_Block(
				            b16, 1,                                                 // format
				            b16, (int16_t)entry->baseValues[foundIndex].coordinate, // coordinate
				            bkover),
				        bkover);
			} else {
				bk_push(baseValues, // assign a zero value
				        p16,
				        bk_new_Block(b16, 1, // format
				                     b16, 0, // coordinate
				                     bkover),
				        bkover);
			}
		}
		bk_Block *scriptRecord = bk_new_Block(p16, baseValues, // BaseValues
		                                      p16, NULL,       // DefaultMinMax
		                                      b16, 0,          // BaseLangSysCount
		                                      bkover);
		bk_push(baseScriptList, b32, entry->tag, // BaseScriptTag
		        p16, scriptRecord,               // BaseScript
		        bkover);
	}
	FREE(taglist.items);
	return bk_new_Block(p16, baseTagList,    // BaseTagList
	                    p16, baseScriptList, // BaseScriptList
	                    bkover);
}

caryll_Buffer *otfcc_buildBASE(const table_BASE *base, const otfcc_Options *options) {
	if (!base) return NULL;
	bk_Block *root = bk_new_Block(b32, 0x10000,                    // Version
	                              p16, axisToBk(base->horizontal), // HorizAxis
	                              p16, axisToBk(base->vertical),   // VertAxis
	                              bkover);
	return bk_build_Block(root);
}
