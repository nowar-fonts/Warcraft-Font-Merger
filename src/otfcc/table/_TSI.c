#include "_TSI.h"
#include "support/util.h"

static INLINE void initTSIEntry(tsi_Entry *entry) {
	Handle.init(&entry->glyph);
	entry->type = TSI_GLYPH;
	entry->content = NULL;
}
static INLINE void copyTSIEntry(tsi_Entry *dst, const tsi_Entry *src) {
	Handle.copy(&dst->glyph, &src->glyph);
	dst->type = src->type;
	dst->content = sdsdup(src->content);
}
static INLINE void disposeTSIEntry(tsi_Entry *entry) {
	Handle.dispose(&entry->glyph);
	sdsfree(entry->content);
}
caryll_standardType(tsi_Entry, tsi_iEntry, initTSIEntry, copyTSIEntry, disposeTSIEntry);
caryll_standardVectorImpl(table_TSI, tsi_Entry, tsi_iEntry, table_iTSI);

static INLINE bool isValidGID(uint16_t gid, uint32_t tagIndex) {
	if (tagIndex == 'TSI0') {
		return gid != 0xFFFE && gid != 0xFFFC;
	} else {
		return gid < 0xFFFA;
	}
}

table_TSI *otfcc_readTSI(const otfcc_Packet packet, const otfcc_Options *options, uint32_t tagIndex,
                         uint32_t tagText) {
	otfcc_PacketPiece textPart;
	textPart.tag = 0;
	otfcc_PacketPiece indexPart;
	indexPart.tag = 0;
	FOR_TABLE(tagIndex, tableIx) {
		indexPart = tableIx;
	}
	FOR_TABLE(tagText, tableTx) {
		textPart = tableTx;
	}
	if (!textPart.tag || !indexPart.tag) return NULL;

	table_TSI *tsi = table_iTSI.create();
	for (uint32_t j = 0; j * 8 < indexPart.length; j++) {
		uint16_t gid = read_16u(indexPart.data + j * 8);
		uint32_t textLength = read_16u(indexPart.data + j * 8 + 2);
		uint32_t textOffset = read_32u(indexPart.data + j * 8 + 4);
		if (!isValidGID(gid, tagIndex) || textOffset >= textPart.length || !textLength) continue;
		// acquire next valid entry to retreve length of text of this entry
		uint32_t predictedTextLength = textPart.length - textOffset;
		for (glyphid_t k = j + 1; k * 8 < indexPart.length; k++) {
			uint16_t gidK = read_16u(indexPart.data + k * 8);
			uint32_t textOffsetK = read_32u(indexPart.data + k * 8 + 4);
			if (gidK != 0xFFFE && textOffsetK < textPart.length && textOffsetK > textOffset) {
				predictedTextLength = textOffsetK - textOffset;
				break;
			}
		}
		if (textLength >= 0x8000) { textLength = predictedTextLength; }
		// push entry
		tsi_Entry entry;
		switch (gid) {
			case 0xFFFA:
				entry.type = TSI_PREP;
				Handle.init(&entry.glyph);
				break;
			case 0xFFFB:
				entry.type = TSI_CVT;
				Handle.init(&entry.glyph);
				break;
			case 0xFFFD:
				entry.type = TSI_FPGM;
				Handle.init(&entry.glyph);
				break;
			default:
				entry.type = TSI_GLYPH;
				entry.glyph = Handle.fromIndex(gid);
				break;
		}
		entry.content = sdsnewlen(textPart.data + textOffset, textLength);
		table_iTSI.push(tsi, entry);
	}
	return tsi;
}

void otfcc_dumpTSI(const table_TSI *tsi, json_value *root, const otfcc_Options *options,
                   const char *tag) {
	if (!tsi) return;
	loggedStep("%s", tag) {
		json_value *_tsi = json_object_new(2);
		json_value *_glyphs = json_object_new(tsi->length);
		foreach (tsi_Entry *entry, *tsi) {
			if (entry->type != TSI_GLYPH) continue;
			json_object_push(
			    _glyphs, entry->glyph.name,
			    json_string_new_length((uint32_t)sdslen(entry->content), entry->content));
		}

		json_value *_extra = json_object_new(tsi->length);
		foreach (tsi_Entry *entry, *tsi) {
			if (entry->type == TSI_GLYPH) continue;
			char *extraKey;
			switch (entry->type) {
				case TSI_CVT:
					extraKey = "cvt";
					break;
				case TSI_FPGM:
					extraKey = "fpgm";
					break;
					break;
				case TSI_PREP:
					extraKey = "prep";
					break;
				default:
					extraKey = "reserved";
					break;
			}
			json_object_push(
			    _extra, extraKey,
			    json_string_new_length((uint32_t)sdslen(entry->content), entry->content));
		}
		json_object_push(_tsi, "glyphs", _glyphs);
		json_object_push(_tsi, "extra", _extra);
		json_object_push(root, tag, _tsi);
	}
}

table_TSI *otfcc_parseTSI(const json_value *root, const otfcc_Options *options, const char *tag) {
	json_value *_tsi = NULL;
	if (!(_tsi = json_obj_get_type(root, tag, json_object))) return NULL;
	table_TSI *tsi = table_iTSI.create();
	loggedStep("%s", tag) {
		json_value *_glyphs = json_obj_get_type(_tsi, "glyphs", json_object);
		if (_glyphs) {
			for (uint32_t j = 0; j < _glyphs->u.object.length; j++) {
				char *_gid = _glyphs->u.object.values[j].name;
				size_t _gidlen = _glyphs->u.object.values[j].name_length;
				json_value *_content = _glyphs->u.object.values[j].value;
				if (!_content || _content->type != json_string) continue;
				table_iTSI.push(tsi, (tsi_Entry){.type = TSI_GLYPH,
				                                 .glyph = Handle.fromName(sdsnewlen(_gid, _gidlen)),
				                                 .content = sdsnewlen(_content->u.string.ptr,
				                                                      _content->u.string.length)});
			}
		}
		json_value *_extra = json_obj_get_type(_tsi, "extra", json_object);
		if (_extra) {
			for (uint32_t j = 0; j < _extra->u.object.length; j++) {
				char *_key = _extra->u.object.values[j].name;
				json_value *_content = _extra->u.object.values[j].value;
				if (!_content || _content->type != json_string) continue;
				if (strcmp(_key, "cvt") == 0) {
					table_iTSI.push(tsi,
					                (tsi_Entry){.type = TSI_CVT,
					                            .glyph = Handle.empty(),
					                            .content = sdsnewlen(_content->u.string.ptr,
					                                                 _content->u.string.length)});
				} else if (strcmp(_key, "fpgm") == 0) {
					table_iTSI.push(tsi,
					                (tsi_Entry){.type = TSI_FPGM,
					                            .glyph = Handle.empty(),
					                            .content = sdsnewlen(_content->u.string.ptr,
					                                                 _content->u.string.length)});
				} else if (strcmp(_key, "prep") == 0) {
					table_iTSI.push(tsi,
					                (tsi_Entry){.type = TSI_PREP,
					                            .glyph = Handle.empty(),
					                            .content = sdsnewlen(_content->u.string.ptr,
					                                                 _content->u.string.length)});
				}
			}
		}
	}
	return tsi;
}

static glyphid_t propergid(tsi_Entry *entry, const tsi_EntryType type) {
	switch (type) {
		case TSI_CVT:
			return 0xFFFB;
		case TSI_FPGM:
			return 0xFFFD;
		case TSI_PREP:
			return 0xFFFA;
		case TSI_RESERVED_FFFC:
			return 0xFFFC;
		case TSI_GLYPH:
			return entry->glyph.index;
	}
}

static void pushTSIEntries(tsi_BuildTarget *target, const table_TSI *tsi, const tsi_EntryType type,
                           const glyphid_t minN) {
	glyphid_t itemsPushed = 0;
	foreach (tsi_Entry *entry, *tsi) {
		if (entry->type != type) continue;
		size_t lengthSofar = target->textPart->cursor;
		bufwrite_sds(target->textPart, entry->content);
		size_t lengthAfter = target->textPart->cursor;
		bufwrite16b(target->indexPart, propergid(entry, type));
		if (lengthAfter - lengthSofar < 0x8000) {
			bufwrite16b(target->indexPart, lengthAfter - lengthSofar);
		} else {
			bufwrite16b(target->indexPart, 0x8000);
		}
		bufwrite32b(target->indexPart, (uint32_t)lengthSofar);
		itemsPushed += 1;
	}
	while (itemsPushed < minN) {
		bufwrite16b(target->indexPart, propergid(NULL, type));
		bufwrite16b(target->indexPart, 0x0000);
		bufwrite32b(target->indexPart, (uint32_t)target->textPart->cursor);
		itemsPushed += 1;
	}
}

tsi_BuildTarget otfcc_buildTSI(const table_TSI *tsi, const otfcc_Options *options) {

	tsi_BuildTarget target;
	if (!tsi) {
		target.textPart = NULL;
		target.indexPart = NULL;
	} else {
		target.textPart = bufnew();
		target.indexPart = bufnew();

		pushTSIEntries(&target, tsi, TSI_GLYPH, 0);
		// magic
		bufwrite16b(target.indexPart, 0xFFFE);
		bufwrite16b(target.indexPart, 0x0000);
		bufwrite32b(target.indexPart, 0xABFC1F34);
		pushTSIEntries(&target, tsi, TSI_PREP, 1);
		pushTSIEntries(&target, tsi, TSI_CVT, 1);
		pushTSIEntries(&target, tsi, TSI_RESERVED_FFFC, 1);
		pushTSIEntries(&target, tsi, TSI_FPGM, 1);
	}
	return target;
}
