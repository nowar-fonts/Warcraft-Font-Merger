#include "head.h"

#include "support/util.h"

static INLINE void initHead(table_head *head) {
	memset(head, 0, sizeof(*head));
	head->magicNumber = 0x5f0f3cf5;
	head->unitsPerEm = 1000;
}
static INLINE void disposeHead(table_head *head) {
	// trivial
}
caryll_standardRefType(table_head, table_iHead, initHead, disposeHead);

table_head *otfcc_readHead(const otfcc_Packet packet, const otfcc_Options *options) {
	FOR_TABLE('head', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;

		if (length < 54) {
			logWarning("table 'head' corrupted.\n");
		} else {
			table_head *head;
			NEW(head);
			head->version = read_32s(data);
			head->fontRevision = read_32u(data + 4);
			head->checkSumAdjustment = read_32u(data + 8);
			head->magicNumber = read_32u(data + 12);
			head->flags = read_16u(data + 16);
			head->unitsPerEm = read_16u(data + 18);
			head->created = read_64u(data + 20);
			head->modified = read_64u(data + 28);
			head->xMin = read_16u(data + 36);
			head->yMin = read_16u(data + 38);
			head->xMax = read_16u(data + 40);
			head->yMax = read_16u(data + 42);
			head->macStyle = read_16u(data + 44);
			head->lowestRecPPEM = read_16u(data + 46);
			head->fontDirectoryHint = read_16u(data + 48);
			head->indexToLocFormat = read_16u(data + 50);
			head->glyphDataFormat = read_16u(data + 52);
			return head;
		}
	}
	return NULL;
}

static const char *headFlagsLabels[] = {"baselineAtY_0",
                                        "lsbAtX_0",
                                        "instrMayDependOnPointSize",
                                        "alwaysUseIntegerSize",
                                        "instrMayAlterAdvanceWidth",
                                        "designedForVertical",
                                        "_reserved1",
                                        "designedForComplexScript",
                                        "hasMetamorphosisEffects",
                                        "containsStrongRTL",
                                        "containsIndicRearrangement",
                                        "fontIsLossless",
                                        "fontIsConverted",
                                        "optimizedForCleartype",
                                        "lastResortFont",
                                        NULL};
static const char *macStyleLabels[] = {"bold",   "italic",    "underline", "outline",
                                       "shadow", "condensed", "extended",  NULL};

void otfcc_dumpHead(const table_head *table, json_value *root, const otfcc_Options *options) {
	if (!table) return;
	loggedStep("head") {
		json_value *head = json_object_new(15);
		json_object_push(head, "version", json_double_new(otfcc_from_fixed(table->version)));
		json_object_push(head, "fontRevision",
		                 json_double_new(otfcc_from_fixed(table->fontRevision)));
		json_object_push(head, "flags", otfcc_dump_flags(table->flags, headFlagsLabels));
		json_object_push(head, "unitsPerEm", json_integer_new(table->unitsPerEm));
		json_object_push(head, "created", json_integer_new(table->created));
		json_object_push(head, "modified", json_integer_new(table->modified));
		json_object_push(head, "xMin", json_integer_new(table->xMin));
		json_object_push(head, "xMax", json_integer_new(table->xMax));
		json_object_push(head, "yMin", json_integer_new(table->yMin));
		json_object_push(head, "yMax", json_integer_new(table->yMax));
		json_object_push(head, "macStyle", otfcc_dump_flags(table->macStyle, macStyleLabels));
		json_object_push(head, "lowestRecPPEM", json_integer_new(table->lowestRecPPEM));
		json_object_push(head, "fontDirectoryHint", json_integer_new(table->fontDirectoryHint));
		json_object_push(head, "indexToLocFormat", json_integer_new(table->indexToLocFormat));
		json_object_push(head, "glyphDataFormat", json_integer_new(table->glyphDataFormat));
		json_object_push(root, "head", head);
	}
}

table_head *otfcc_parseHead(const json_value *root, const otfcc_Options *options) {
	table_head *head = table_iHead.create();
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "head", json_object))) {
		loggedStep("head") {
			head->version = otfcc_to_fixed(json_obj_getnum_fallback(table, "version", 0));
			head->fontRevision = otfcc_to_fixed(json_obj_getnum_fallback(table, "fontRevision", 0));
			head->flags = otfcc_parse_flags(json_obj_get(table, "flags"), headFlagsLabels);
			head->unitsPerEm = json_obj_getnum_fallback(table, "unitsPerEm", 0);
			head->created = json_obj_getnum_fallback(table, "created", 0);
			head->modified = json_obj_getnum_fallback(table, "modified", 0);
			head->xMin = json_obj_getnum_fallback(table, "xMin", 0);
			head->xMax = json_obj_getnum_fallback(table, "xMax", 0);
			head->yMin = json_obj_getnum_fallback(table, "yMin", 0);
			head->yMax = json_obj_getnum_fallback(table, "yMax", 0);
			head->macStyle = otfcc_parse_flags(json_obj_get(table, "macStyle"), macStyleLabels);
			head->lowestRecPPEM = json_obj_getnum_fallback(table, "lowestRecPPEM", 0);
			head->fontDirectoryHint = json_obj_getnum_fallback(table, "fontDirectoryHint", 0);
			head->indexToLocFormat = json_obj_getnum_fallback(table, "indexToLocFormat", 0);
			head->glyphDataFormat = json_obj_getnum_fallback(table, "glyphDataFormat", 0);
		}
	}
	return head;
}

caryll_Buffer *otfcc_buildHead(const table_head *head, const otfcc_Options *options) {
	if (!head) return NULL;
	caryll_Buffer *buf = bufnew();
	bufwrite32b(buf, head->version);
	bufwrite32b(buf, head->fontRevision);
	bufwrite32b(buf, head->checkSumAdjustment);
	bufwrite32b(buf, head->magicNumber);
	bufwrite16b(buf, head->flags);
	bufwrite16b(buf, head->unitsPerEm);
	bufwrite64b(buf, head->created);
	bufwrite64b(buf, head->modified);
	bufwrite16b(buf, head->xMin);
	bufwrite16b(buf, head->yMin);
	bufwrite16b(buf, head->xMax);
	bufwrite16b(buf, head->yMax);
	bufwrite16b(buf, head->macStyle);
	bufwrite16b(buf, head->lowestRecPPEM);
	bufwrite16b(buf, head->fontDirectoryHint);
	bufwrite16b(buf, head->indexToLocFormat);
	bufwrite16b(buf, head->glyphDataFormat);
	return buf;
}
