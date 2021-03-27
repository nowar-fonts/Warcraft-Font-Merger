#include "COLR.h"

#include "support/util.h"
#include "bk/bkgraph.h"

static INLINE void initLayer(colr_Layer *layer) {
	Handle.init(&layer->glyph);
}
static INLINE void copyLayer(colr_Layer *dst, const colr_Layer *src) {
	Handle.copy(&dst->glyph, &src->glyph);
	dst->paletteIndex = src->paletteIndex;
}
static INLINE void disposeLayer(colr_Layer *layer) {
	Handle.dispose(&layer->glyph);
}
caryll_standardType(colr_Layer, colr_iLayer, initLayer, copyLayer, disposeLayer);
caryll_standardVectorImpl(colr_LayerList, colr_Layer, colr_iLayer, colr_iLayerList);

static INLINE void initMapping(colr_Mapping *mapping) {
	Handle.init(&mapping->glyph);
	colr_iLayerList.init(&mapping->layers);
}
static INLINE void copyMapping(colr_Mapping *dst, const colr_Mapping *src) {
	Handle.copy(&dst->glyph, &src->glyph);
	colr_iLayerList.copy(&dst->layers, &src->layers);
}
static INLINE void disposeMapping(colr_Mapping *mapping) {
	Handle.dispose(&mapping->glyph);
	colr_iLayerList.dispose(&mapping->layers);
}
caryll_standardType(colr_Mapping, colr_iMapping, initMapping, copyMapping, disposeMapping);
caryll_standardVectorImpl(table_COLR, colr_Mapping, colr_iMapping, table_iCOLR);

static const size_t baseGlyphRecLength = 6;
static const size_t layerRecLength = 4;

table_COLR *otfcc_readCOLR(const otfcc_Packet packet, const otfcc_Options *options) {
	table_COLR *colr = NULL;
	FOR_TABLE('COLR', table) {
		if (table.length < 14) goto FAIL;
		uint16_t numBaseGlyphRecords = read_16u(table.data + 2);
		uint16_t numLayerRecords = read_16u(table.data + 12);
		uint32_t offsetBaseGlyphRecord = read_32u(table.data + 4);
		uint32_t offsetLayerRecord = read_32u(table.data + 8);
		if (table.length < offsetBaseGlyphRecord + baseGlyphRecLength * numBaseGlyphRecords)
			goto FAIL;
		if (table.length < offsetLayerRecord + layerRecLength * numLayerRecords) goto FAIL;

		// parse layer data
		glyphid_t *gids;
		colorid_t *colors;
		NEW(gids, numLayerRecords);
		NEW(colors, numLayerRecords);
		for (glyphid_t j = 0; j < numLayerRecords; j++) {
			gids[j] = read_16u(table.data + offsetLayerRecord + layerRecLength * j);
			colors[j] = read_16u(table.data + offsetLayerRecord + layerRecLength * j + 2);
		}
		// parse decomposition data
		colr = table_iCOLR.create();
		for (glyphid_t j = 0; j < numBaseGlyphRecords; j++) {
			colr_Mapping mapping;
			colr_iMapping.init(&mapping);
			uint16_t gid = read_16u(table.data + offsetBaseGlyphRecord + baseGlyphRecLength * j);
			uint16_t firstLayerIndex =
			    read_16u(table.data + offsetBaseGlyphRecord + baseGlyphRecLength * j + 2);
			uint16_t numLayers =
			    read_16u(table.data + offsetBaseGlyphRecord + baseGlyphRecLength * j + 4);

			glyph_handle baseGlyph = Handle.fromIndex(gid);
			Handle.move(&mapping.glyph, &baseGlyph);
			for (glyphid_t k = 0; k < numLayers; k++) {
				if (k + firstLayerIndex < numLayerRecords) {
					colr_iLayerList.push(&mapping.layers,
					                     (colr_Layer){
					                         .glyph = Handle.fromIndex(gids[k + firstLayerIndex]),
					                         .paletteIndex = colors[k + firstLayerIndex],
					                     });
				}
			}
			table_iCOLR.push(colr, mapping);
		}
		return colr;
	FAIL:
		logWarning("Table 'COLR' corrupted.\n");
		table_iCOLR.free(colr);
		colr = NULL;
	}
	return colr;
}

void otfcc_dumpCOLR(const table_COLR *colr, json_value *root, const otfcc_Options *options) {
	if (!colr) return;
	loggedStep("COLR") {
		json_value *_colr = json_array_new(colr->length);
		foreach (colr_Mapping *mapping, *colr) {
			json_value *_map = json_object_new(2);
			json_object_push(_map, "from", json_string_new(mapping->glyph.name));
			json_value *_layers = json_array_new(mapping->layers.length);
			foreach (colr_Layer *layer, mapping->layers) {
				json_value *_layer = json_object_new(2);
				json_object_push(_layer, "layer", json_string_new(layer->glyph.name));
				json_object_push(_layer, "paletteIndex", json_integer_new(layer->paletteIndex));
				json_array_push(_layers, _layer);
			}
			json_object_push(_map, "to", preserialize(_layers));
			json_array_push(_colr, _map);
		}
		json_object_push(root, "COLR", _colr);
	}
}

table_COLR *otfcc_parseCOLR(const json_value *root, const otfcc_Options *options) {
	json_value *_colr = NULL;
	if (!(_colr = json_obj_get_type(root, "COLR", json_array))) return NULL;
	table_COLR *colr = table_iCOLR.create();
	loggedStep("COLR") {
		for (glyphid_t j = 0; j < _colr->u.array.length; j++) {
			json_value *_mapping = _colr->u.array.values[j];
			if (!_mapping || _mapping->type != json_object) continue;
			json_value *_baseglyph = json_obj_get_type(_mapping, "from", json_string);
			json_value *_layers = json_obj_get_type(_mapping, "to", json_array);
			if (!_baseglyph || !_layers) continue;

			colr_Mapping m;
			colr_iMapping.init(&m);
			m.glyph =
			    Handle.fromName(sdsnewlen(_baseglyph->u.string.ptr, _baseglyph->u.string.length));
			for (glyphid_t k = 0; k < _layers->u.array.length; k++) {
				json_value *_layer = _layers->u.array.values[k];
				if (!_layer || _layer->type != json_object) continue;
				json_value *_layerglyph = json_obj_get_type(_layer, "layer", json_string);
				if (!_layerglyph) continue;
				colr_iLayerList.push(
				    &m.layers,
				    (colr_Layer){
				        .glyph = Handle.fromName(
				            sdsnewlen(_layerglyph->u.string.ptr, _layerglyph->u.string.length)),
				        .paletteIndex = json_obj_getint_fallback(_layer, "paletteIndex", 0xFFFF),
				    });
			}
			table_iCOLR.push(colr, m);
		}
	}
	return colr;
}

static int byGID(const colr_Mapping *a, const colr_Mapping *b) {
	return a->glyph.index - b->glyph.index;
}

caryll_Buffer *otfcc_buildCOLR(const table_COLR *_colr, const otfcc_Options *options) {
	if (!_colr || !_colr->length) return NULL;

	// sort base defs
	table_COLR colr;
	table_iCOLR.copy(&colr, _colr);
	table_iCOLR.sort(&colr, byGID);

	glyphid_t currentLayerIndex = 0;
	bk_Block *layerRecords = bk_new_Block(bkover);
	bk_Block *baseRecords = bk_new_Block(bkover);
	foreach (colr_Mapping *mapping, colr) {
		bk_push(baseRecords, b16, mapping->glyph.index, // GID
		        b16, currentLayerIndex,                 // firstLayerIndex
		        b16, mapping->layers.length,            // numLayers
		        bkover);
		foreach (colr_Layer *layer, mapping->layers) {
			bk_push(layerRecords, b16, layer->glyph.index, // GID
			        b16, layer->paletteIndex,              // paletteIndex
			        bkover);
			currentLayerIndex += 1;
		}
	}

	bk_Block *root = bk_new_Block(b16, 0,                 // Version
	                              b16, colr.length,       // numBaseGlyphRecords
	                              p32, baseRecords,       // offsetBaseGlyphRecord
	                              p32, layerRecords,      // offsetLayerRecord
	                              b16, currentLayerIndex, // numLayerRecords
	                              bkover);
	table_iCOLR.dispose(&colr);
	return bk_build_Block(root);
}
