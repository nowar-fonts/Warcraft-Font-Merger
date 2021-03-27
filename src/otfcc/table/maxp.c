#include "maxp.h"

#include "support/util.h"

static INLINE void initMaxp(table_maxp *maxp) {
	memset(maxp, 0, sizeof(*maxp));
	maxp->version = 0x10000;
}
static INLINE void disposeMaxp(MOVE table_maxp *maxp) {
	// trivial
}
caryll_standardRefType(table_maxp, table_iMaxp, initMaxp, disposeMaxp);

table_maxp *otfcc_readMaxp(const otfcc_Packet packet, const otfcc_Options *options) {
	FOR_TABLE('maxp', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;

		if (length != 32 && length != 6) {
			logWarning("table 'maxp' corrupted.\n");
		} else {
			table_maxp *maxp = table_iMaxp.create();
			maxp->version = read_32s(data);
			maxp->numGlyphs = read_16u(data + 4);
			if (maxp->version == 0x00010000) { // TrueType Format 1
				maxp->maxPoints = read_16u(data + 6);
				maxp->maxContours = read_16u(data + 8);
				maxp->maxCompositePoints = read_16u(data + 10);
				maxp->maxCompositeContours = read_16u(data + 12);
				maxp->maxZones = read_16u(data + 14);
				maxp->maxTwilightPoints = read_16u(data + 16);
				maxp->maxStorage = read_16u(data + 18);
				maxp->maxFunctionDefs = read_16u(data + 20);
				maxp->maxInstructionDefs = read_16u(data + 22);
				maxp->maxStackElements = read_16u(data + 24);
				maxp->maxSizeOfInstructions = read_16u(data + 26);
				maxp->maxComponentElements = read_16u(data + 28);
				maxp->maxComponentDepth = read_16u(data + 30);
			} else { // CFF OTF Format 0.5
				maxp->maxPoints = 0;
				maxp->maxContours = 0;
				maxp->maxCompositePoints = 0;
				maxp->maxCompositeContours = 0;
				maxp->maxZones = 0;
				maxp->maxTwilightPoints = 0;
				maxp->maxStorage = 0;
				maxp->maxFunctionDefs = 0;
				maxp->maxInstructionDefs = 0;
				maxp->maxStackElements = 0;
				maxp->maxSizeOfInstructions = 0;
				maxp->maxComponentElements = 0;
				maxp->maxComponentDepth = 0;
			}
			return maxp;
		}
	}
	return NULL;
}

void otfcc_dumpMaxp(const table_maxp *table, json_value *root, const otfcc_Options *options) {
	if (!table) return;
	loggedStep("maxp") {
		json_value *maxp = json_object_new(15);
		json_object_push(maxp, "version", json_double_new(otfcc_from_fixed(table->version)));
		json_object_push(maxp, "numGlyphs", json_integer_new(table->numGlyphs));
		json_object_push(maxp, "maxPoints", json_integer_new(table->maxPoints));
		json_object_push(maxp, "maxContours", json_integer_new(table->maxContours));
		json_object_push(maxp, "maxCompositePoints", json_integer_new(table->maxCompositePoints));
		json_object_push(maxp, "maxCompositeContours",
		                 json_integer_new(table->maxCompositeContours));
		json_object_push(maxp, "maxZones", json_integer_new(table->maxZones));
		json_object_push(maxp, "maxTwilightPoints", json_integer_new(table->maxTwilightPoints));
		json_object_push(maxp, "maxStorage", json_integer_new(table->maxStorage));
		json_object_push(maxp, "maxFunctionDefs", json_integer_new(table->maxFunctionDefs));
		json_object_push(maxp, "maxInstructionDefs", json_integer_new(table->maxInstructionDefs));
		json_object_push(maxp, "maxStackElements", json_integer_new(table->maxStackElements));
		json_object_push(maxp, "maxSizeOfInstructions",
		                 json_integer_new(table->maxSizeOfInstructions));
		json_object_push(maxp, "maxComponentElements",
		                 json_integer_new(table->maxComponentElements));
		json_object_push(maxp, "maxComponentDepth", json_integer_new(table->maxComponentDepth));
		json_object_push(root, "maxp", maxp);
	}
}

table_maxp *otfcc_parseMaxp(const json_value *root, const otfcc_Options *options) {
	table_maxp *maxp = table_iMaxp.create();
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "maxp", json_object))) {
		loggedStep("maxp") {
			maxp->version = otfcc_to_fixed(json_obj_getnum(table, "version"));
			maxp->numGlyphs = json_obj_getnum(table, "numGlyphs");
			maxp->maxZones = json_obj_getnum(table, "maxZones");
			maxp->maxTwilightPoints = json_obj_getnum(table, "maxTwilightPoints");
			maxp->maxStorage = json_obj_getnum(table, "maxStorage");
			maxp->maxFunctionDefs = json_obj_getnum(table, "maxFunctionDefs");
			maxp->maxInstructionDefs = json_obj_getnum(table, "maxInstructionDefs");
			maxp->maxStackElements = json_obj_getnum(table, "maxStackElements");
		}
	}
	return maxp;
}

caryll_Buffer *otfcc_buildMaxp(const table_maxp *maxp, const otfcc_Options *options) {
	if (!maxp) return NULL;
	caryll_Buffer *buf = bufnew();
	bufwrite32b(buf, maxp->version);
	bufwrite16b(buf, maxp->numGlyphs);
	if (maxp->version > 0x00005000) {
		bufwrite16b(buf, maxp->maxPoints);
		bufwrite16b(buf, maxp->maxContours);
		bufwrite16b(buf, maxp->maxCompositePoints);
		bufwrite16b(buf, maxp->maxCompositeContours);
		bufwrite16b(buf, maxp->maxZones);
		bufwrite16b(buf, maxp->maxTwilightPoints);
		bufwrite16b(buf, maxp->maxStorage);
		bufwrite16b(buf, maxp->maxFunctionDefs);
		bufwrite16b(buf, maxp->maxInstructionDefs);
		bufwrite16b(buf, maxp->maxStackElements);
		bufwrite16b(buf, maxp->maxSizeOfInstructions);
		bufwrite16b(buf, maxp->maxComponentElements);
		bufwrite16b(buf, maxp->maxComponentDepth);
	}
	return buf;
}
