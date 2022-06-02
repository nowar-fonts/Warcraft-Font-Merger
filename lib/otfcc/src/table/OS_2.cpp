#include "OS_2.h"
#include "support/util.h"

static INLINE void initOS2(table_OS_2 *table) {
	memset(table, 0, sizeof(*table));
	table->version = 4;
}
static INLINE void disposeOS2(MOVE table_OS_2 *table) {
	// trivial
}
caryll_standardRefType(table_OS_2, table_iOS_2, initOS2, disposeOS2);

table_OS_2 *otfcc_readOS_2(const otfcc_Packet packet, const otfcc_Options *options) {
	table_OS_2 *os_2 = NULL;
	FOR_TABLE('OS/2', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		if (length < 2) goto OS_2_CORRUPTED;
		os_2 = table_iOS_2.create();
		os_2->version = read_16u(data);
		// version 0 (Apple's TrueType)
		if (length < 68) goto OS_2_CORRUPTED;
		{
			os_2->xAvgCharWidth = read_16u(data + 2);
			os_2->usWeightClass = read_16u(data + 4);
			os_2->usWidthClass = read_16u(data + 6);
			os_2->fsType = read_16u(data + 8);
			os_2->ySubscriptXSize = read_16u(data + 10);
			os_2->ySubscriptYSize = read_16u(data + 12);
			os_2->ySubscriptXOffset = read_16u(data + 14);
			os_2->ySubscriptYOffset = read_16u(data + 16);
			os_2->ySupscriptXSize = read_16u(data + 18);
			os_2->ySupscriptYSize = read_16u(data + 20);
			os_2->ySupscriptXOffset = read_16u(data + 22);
			os_2->ySupscriptYOffset = read_16u(data + 24);
			os_2->yStrikeoutSize = read_16u(data + 26);
			os_2->yStrikeoutPosition = read_16u(data + 28);
			os_2->sFamilyClass = read_16u(data + 30);
			memcpy(os_2->panose, data + 32, 10);
			os_2->ulUnicodeRange1 = read_32u(data + 42);
			os_2->ulUnicodeRange2 = read_32u(data + 46);
			os_2->ulUnicodeRange3 = read_32u(data + 50);
			os_2->ulUnicodeRange4 = read_32u(data + 54);
			memcpy(os_2->achVendID, data + 58, 4);
			os_2->fsSelection = read_16u(data + 62);
			os_2->usFirstCharIndex = read_16u(data + 64);
			os_2->usLastCharIndex = read_16u(data + 66);
		}
		// version 0 (OpenType)
		if (length >= 78) {
			os_2->sTypoAscender = read_16s(data + 68);
			os_2->sTypoDescender = read_16s(data + 70);
			os_2->sTypoLineGap = read_16s(data + 72);
			os_2->usWinAscent = read_16u(data + 74);
			os_2->usWinDescent = read_16u(data + 76);
		}
		// version 1
		if (os_2->version >= 1 && length < 86) goto OS_2_CORRUPTED;
		if (os_2->version >= 1) {
			os_2->ulCodePageRange1 = read_32u(data + 78);
			os_2->ulCodePageRange2 = read_32u(data + 82);
		}
		// version 2, 3, 4
		if (os_2->version >= 2 && length < 96) goto OS_2_CORRUPTED;
		if (os_2->version >= 2) {
			os_2->sxHeight = read_16s(data + 86);
			os_2->sCapHeight = read_16s(data + 88);
			os_2->usDefaultChar = read_16u(data + 90);
			os_2->usBreakChar = read_16u(data + 92);
			os_2->usMaxContext = read_16u(data + 94);
		}
		// version 5
		if (os_2->version >= 5 && length < 100) goto OS_2_CORRUPTED;
		if (os_2->version >= 5) {
			os_2->usLowerOpticalPointSize = read_16u(data + 96);
			os_2->usLowerOpticalPointSize = read_16u(data + 98);
		}
		return os_2;

	OS_2_CORRUPTED:
		logWarning("table 'OS/2' corrupted.\n");
		if (os_2 != NULL) FREE(os_2);
	}
	return NULL;
}

const char *fsTypeLabels[] = {"_reserved1",
                              "restrictedLicense",
                              "previewPrintLicense",
                              "editableEmbedding",
                              "_reserved2",
                              "_reserved3",
                              "_reserved4",
                              "_reserved5",
                              "noSubsetting",
                              "bitmapEmbeddingOnly",
                              NULL};
const char *fsSelectionLabels[] = {"italic",    "underscore", "negative", "outlined",
                                   "strikeout", "bold",       "regular",  "useTypoMetrics",
                                   "wws",       "oblique",    NULL};
const char *codePageLabels1[] = {
    "latin1",        "latin2",     "cyrillic", "greek",  "turkish", "hebrew", "arabic",
    "windowsBaltic", "vietnamese", "ansi1",    "ansi2",  "ansi3",   "ansi4",  "ansi5",
    "ansi6",         "ansi7",      "thai",     "jis",    "gbk",     "korean", "big5",
    "koreanJohab",   "oem1",       "oem2",     "oem3",   "oem4",    "oem5",   "oem6",
    "oem7",          "macRoman",   "oem",      "symbol", NULL};
const char *codePageLabels2[] = {"oem8",  "oem9",  "oem10", "oem11", "oem12", "oem13", "oem14",
                                 "oem15", "oem16", "oem17", "oem18", "oem19", "oem20", "oem21",
                                 "oem22", "oem23", "cp869", "cp866", "cp865", "cp864", "cp863",
                                 "cp862", "cp861", "cp860", "cp857", "cp855", "cp852", "cp775",
                                 "cp737", "cp708", "cp850", "ascii", NULL};
const char *unicodeRangeLabels1[] = {"Basic_Latin",
                                     "Latin_1_Supplement",
                                     "Latin_Extended_A",
                                     "Latin_Extended_B",
                                     "Phonetics",
                                     "Spacing_Modifiers",
                                     "Combining_Diacritical_Marks",
                                     "Greek_and_Coptic",
                                     "Coptic",
                                     "Cyrillic",
                                     "Armenian",
                                     "Hebrew",
                                     "Vai",
                                     "Arabic",
                                     "NKo",
                                     "Devanagari",
                                     "Bengali",
                                     "Gurmukhi",
                                     "Gujarati",
                                     "Oriya",
                                     "Tamil",
                                     "Telugu",
                                     "Kannada",
                                     "Malayalam",
                                     "Thai",
                                     "Lao",
                                     "Georgian",
                                     "Balinese",
                                     "Hangul_Jamo",
                                     "Latin_Extended_Additional",
                                     "Greek_Extended",
                                     "Punctuations",
                                     NULL};
const char *unicodeRangeLabels2[] = {"Superscripts_And_Subscripts",
                                     "Currency_Symbols",
                                     "Combining_Diacritical_Marks_For_Symbols",
                                     "Letterlike_Symbols",
                                     "Number_Forms",
                                     "Arrows",
                                     "Mathematical_Operators",
                                     "Miscellaneous_Technical",
                                     "Control_Pictures",
                                     "Optical_Character_Recognition",
                                     "Enclosed_Alphanumerics",
                                     "Box_Drawing",
                                     "Block_Elements",
                                     "Geometric_Shapes",
                                     "Miscellaneous_Symbols",
                                     "Dingbats",
                                     "CJK_Symbols_And_Punctuation",
                                     "Hiragana",
                                     "Katakana",
                                     "Bopomofo",
                                     "Hangul_Compatibility_Jamo",
                                     "Phags_pa",
                                     "Enclosed_CJK_Letters_And_Months",
                                     "CJK_Compatibility",
                                     "Hangul_Syllables",
                                     "Non_Plane_0",
                                     "Phoenician",
                                     "CJK_Unified_Ideographs",
                                     "Private_Use_Area_p0",
                                     "CJK_Strokes",
                                     "Alphabetic_Presentation_Forms",
                                     "Arabic_Presentation_Forms_A",
                                     NULL};
const char *unicodeRangeLabels3[] = {"Combining_Half_Marks",
                                     "Vertical_Forms_and_CJK_Compatibility_Forms",
                                     "Small_Form_Variants",
                                     "Arabic_Presentation_Forms_B",
                                     "Halfwidth_And_Fullwidth_Forms",
                                     "Specials",
                                     "Tibetan",
                                     "Syriac",
                                     "Thaana",
                                     "Sinhala",
                                     "Myanmar",
                                     "Ethiopic",
                                     "Cherokee",
                                     "Unified_Canadian_Aboriginal_Syllabics",
                                     "Ogham",
                                     "Runic",
                                     "Khmer",
                                     "Mongolian",
                                     "Braille_Patterns",
                                     "Yi_Syllables",
                                     "Tagalog",
                                     "Old_Italic",
                                     "Gothic",
                                     "Deseret",
                                     "Musical_Symbols",
                                     "Mathematical_Alphanumeric_Symbols",
                                     "Private_Use_p15_and_p16",
                                     "Variation_Selectors",
                                     "Tags",
                                     "Limbu",
                                     "Tai_Le",
                                     "New_Tai_Lue",
                                     NULL};
const char *unicodeRangeLabels4[] = {"Buginese",
                                     "Glagolitic",
                                     "Tifinagh",
                                     "Yijing_Hexagram_Symbols",
                                     "Syloti_Nagri",
                                     "Linear_B_Syllabary_Ideograms_and_Aegean_Numbers",
                                     "Ancient_Greek_Numbers",
                                     "Ugaritic",
                                     "Old_Persian",
                                     "Shavian",
                                     "Osmanya",
                                     "Cypriot_Syllabary",
                                     "Kharoshthi",
                                     "Tai_Xuan_Jing_Symbols",
                                     "Cuneiform",
                                     "Counting_Rod_Numerals",
                                     "Sundanese",
                                     "Lepcha",
                                     "Ol_Chiki",
                                     "Saurashtra",
                                     "Kayah_Li",
                                     "Rejang",
                                     "Cham",
                                     "Ancient_Symbols",
                                     "Phaistos_Disc",
                                     "Carian_and_Lycian",
                                     "Domino_and_Mahjong_Tiles",
                                     NULL};

void otfcc_dumpOS_2(const table_OS_2 *table, json_value *root, const otfcc_Options *options) {
	if (!table) return;
	loggedStep("OS/2") {
		json_value *os_2 = json_object_new(30);
		json_object_push(os_2, "version", json_integer_new(table->version));
		json_object_push(os_2, "xAvgCharWidth", json_integer_new(table->xAvgCharWidth));
		json_object_push(os_2, "usWeightClass", json_integer_new(table->usWeightClass));
		json_object_push(os_2, "usWidthClass", json_integer_new(table->usWidthClass));
		json_object_push(os_2, "fsType", otfcc_dump_flags(table->fsType, fsTypeLabels));
		json_object_push(os_2, "ySubscriptXSize", json_integer_new(table->ySubscriptXSize));
		json_object_push(os_2, "ySubscriptYSize", json_integer_new(table->ySubscriptYSize));
		json_object_push(os_2, "ySubscriptXOffset", json_integer_new(table->ySubscriptXOffset));
		json_object_push(os_2, "ySubscriptYOffset", json_integer_new(table->ySubscriptYOffset));
		json_object_push(os_2, "ySupscriptXSize", json_integer_new(table->ySupscriptXSize));
		json_object_push(os_2, "ySupscriptYSize", json_integer_new(table->ySupscriptYSize));
		json_object_push(os_2, "ySupscriptXOffset", json_integer_new(table->ySupscriptXOffset));
		json_object_push(os_2, "ySupscriptYOffset", json_integer_new(table->ySupscriptYOffset));
		json_object_push(os_2, "yStrikeoutSize", json_integer_new(table->yStrikeoutSize));
		json_object_push(os_2, "yStrikeoutPosition", json_integer_new(table->yStrikeoutPosition));
		json_object_push(os_2, "sFamilyClass", json_integer_new(table->sFamilyClass));

		json_value *panose = json_array_new(10);
		for (uint8_t j = 0; j < 10; j++) {
			json_array_push(panose, json_integer_new(table->panose[j]));
		}
		json_object_push(os_2, "panose", panose);

		json_object_push(os_2, "ulUnicodeRange1",
		                 otfcc_dump_flags(table->ulUnicodeRange1, unicodeRangeLabels1));
		json_object_push(os_2, "ulUnicodeRange2",
		                 otfcc_dump_flags(table->ulUnicodeRange2, unicodeRangeLabels2));
		json_object_push(os_2, "ulUnicodeRange3",
		                 otfcc_dump_flags(table->ulUnicodeRange3, unicodeRangeLabels3));
		json_object_push(os_2, "ulUnicodeRange4",
		                 otfcc_dump_flags(table->ulUnicodeRange4, unicodeRangeLabels4));

		sds vendorid = sdsnewlen(table->achVendID, 4);
		json_object_push(os_2, "achVendID", json_string_new(vendorid));
		sdsfree(vendorid);

		json_object_push(os_2, "fsSelection",
		                 otfcc_dump_flags(table->fsSelection, fsSelectionLabels));
		json_object_push(os_2, "usFirstCharIndex", json_integer_new(table->usFirstCharIndex));
		json_object_push(os_2, "usLastCharIndex", json_integer_new(table->usLastCharIndex));
		json_object_push(os_2, "sTypoAscender", json_integer_new(table->sTypoAscender));
		json_object_push(os_2, "sTypoDescender", json_integer_new(table->sTypoDescender));
		json_object_push(os_2, "sTypoLineGap", json_integer_new(table->sTypoLineGap));
		json_object_push(os_2, "usWinAscent", json_integer_new(table->usWinAscent));
		json_object_push(os_2, "usWinDescent", json_integer_new(table->usWinDescent));
		json_object_push(os_2, "ulCodePageRange1",
		                 otfcc_dump_flags(table->ulCodePageRange1, codePageLabels1));
		json_object_push(os_2, "ulCodePageRange2",
		                 otfcc_dump_flags(table->ulCodePageRange2, codePageLabels2));
		json_object_push(os_2, "sxHeight", json_integer_new(table->sxHeight));
		json_object_push(os_2, "sCapHeight", json_integer_new(table->sCapHeight));
		json_object_push(os_2, "usDefaultChar", json_integer_new(table->usDefaultChar));
		json_object_push(os_2, "usBreakChar", json_integer_new(table->usBreakChar));
		json_object_push(os_2, "usMaxContext", json_integer_new(table->usMaxContext));
		json_object_push(os_2, "usLowerOpticalPointSize",
		                 json_integer_new(table->usLowerOpticalPointSize));
		json_object_push(os_2, "usUpperOpticalPointSize",
		                 json_integer_new(table->usUpperOpticalPointSize));
		json_object_push(root, "OS_2", os_2);
	}
}

table_OS_2 *otfcc_parseOS_2(const json_value *root, const otfcc_Options *options) {
	table_OS_2 *os_2 = table_iOS_2.create();
	if (!os_2) return NULL;
	json_value *table = NULL;
	if ((table = json_obj_get_type(root, "OS_2", json_object))) {
		loggedStep("OS/2") {
			os_2->version = json_obj_getnum_fallback(table, "version", 0);
			os_2->xAvgCharWidth = json_obj_getnum_fallback(table, "xAvgCharWidth", 0);
			os_2->usWeightClass = json_obj_getnum_fallback(table, "usWeightClass", 0);
			os_2->usWidthClass = json_obj_getnum_fallback(table, "usWidthClass", 0);
			os_2->fsType = otfcc_parse_flags(json_obj_get(table, "fsType"), fsTypeLabels);
			os_2->ySubscriptXSize = json_obj_getnum_fallback(table, "ySubscriptXSize", 0);
			os_2->ySubscriptYSize = json_obj_getnum_fallback(table, "ySubscriptYSize", 0);
			os_2->ySubscriptXOffset = json_obj_getnum_fallback(table, "ySubscriptXOffset", 0);
			os_2->ySubscriptYOffset = json_obj_getnum_fallback(table, "ySubscriptYOffset", 0);
			os_2->ySupscriptXSize = json_obj_getnum_fallback(table, "ySupscriptXSize", 0);
			os_2->ySupscriptYSize = json_obj_getnum_fallback(table, "ySupscriptYSize", 0);
			os_2->ySupscriptXOffset = json_obj_getnum_fallback(table, "ySupscriptXOffset", 0);
			os_2->ySupscriptYOffset = json_obj_getnum_fallback(table, "ySupscriptYOffset", 0);
			os_2->yStrikeoutSize = json_obj_getnum_fallback(table, "yStrikeoutSize", 0);
			os_2->yStrikeoutPosition = json_obj_getnum_fallback(table, "yStrikeoutPosition", 0);
			os_2->sFamilyClass = json_obj_getnum_fallback(table, "sFamilyClass", 0);
			os_2->fsSelection =
			    otfcc_parse_flags(json_obj_get(table, "fsSelection"), fsSelectionLabels);
			os_2->usFirstCharIndex = json_obj_getnum_fallback(table, "usFirstCharIndex", 0);
			os_2->usLastCharIndex = json_obj_getnum_fallback(table, "usLastCharIndex", 0);
			os_2->sTypoAscender = json_obj_getnum_fallback(table, "sTypoAscender", 0);
			os_2->sTypoDescender = json_obj_getnum_fallback(table, "sTypoDescender", 0);
			os_2->sTypoLineGap = json_obj_getnum_fallback(table, "sTypoLineGap", 0);
			os_2->usWinAscent = json_obj_getnum_fallback(table, "usWinAscent", 0);
			os_2->usWinDescent = json_obj_getnum_fallback(table, "usWinDescent", 0);
			os_2->ulCodePageRange1 =
			    otfcc_parse_flags(json_obj_get(table, "ulCodePageRange1"), codePageLabels1);
			os_2->ulCodePageRange2 =
			    otfcc_parse_flags(json_obj_get(table, "ulCodePageRange2"), codePageLabels2);
			os_2->ulUnicodeRange1 =
			    otfcc_parse_flags(json_obj_get(table, "ulUnicodeRange1"), unicodeRangeLabels1);
			os_2->ulUnicodeRange2 =
			    otfcc_parse_flags(json_obj_get(table, "ulUnicodeRange2"), unicodeRangeLabels2);
			os_2->ulUnicodeRange3 =
			    otfcc_parse_flags(json_obj_get(table, "ulUnicodeRange3"), unicodeRangeLabels3);
			os_2->ulUnicodeRange4 =
			    otfcc_parse_flags(json_obj_get(table, "ulUnicodeRange4"), unicodeRangeLabels4);
			os_2->sxHeight = json_obj_getnum_fallback(table, "sxHeight", 0);
			os_2->sCapHeight = json_obj_getnum_fallback(table, "sCapHeight", 0);
			os_2->usDefaultChar = json_obj_getnum_fallback(table, "usDefaultChar", 0);
			os_2->usBreakChar = json_obj_getnum_fallback(table, "usBreakChar", 0);
			os_2->usMaxContext = json_obj_getnum_fallback(table, "usMaxContext", 0);
			os_2->usLowerOpticalPointSize =
			    json_obj_getnum_fallback(table, "usLowerOpticalPointSize", 0);
			os_2->usUpperOpticalPointSize =
			    json_obj_getnum_fallback(table, "usUpperOpticalPointSize", 0);
			// panose
			json_value *panose = NULL;
			if ((panose = json_obj_get_type(table, "panose", json_array))) {
				for (uint32_t j = 0; j < panose->u.array.length && j < 10; j++) {
					json_value *term = panose->u.array.values[j];
					if (term->type == json_integer) {
						os_2->panose[j] = term->u.integer;
					} else if (term->type == json_double) {
						os_2->panose[j] = term->u.dbl;
					}
				}
			}
			// achVendID
			json_value *vendorid = NULL;
			if ((vendorid = json_obj_get_type(table, "achVendID", json_string))) {
				os_2->achVendID[0] = ' ';
				os_2->achVendID[1] = ' ';
				os_2->achVendID[2] = ' ';
				os_2->achVendID[3] = ' ';
				if (vendorid->u.string.length >= 4) {
					memcpy(os_2->achVendID, vendorid->u.string.ptr, 4);
				} else {
					memcpy(os_2->achVendID, vendorid->u.string.ptr, vendorid->u.string.length);
				}
			}
		}
	}
	if (os_2->version < 1) os_2->version = 1;
	return os_2;
}

caryll_Buffer *otfcc_buildOS_2(const table_OS_2 *os_2, const otfcc_Options *options) {
	if (!os_2) return NULL;
	caryll_Buffer *buf = bufnew();
	bufwrite16b(buf, os_2->version);
	bufwrite16b(buf, os_2->xAvgCharWidth);
	bufwrite16b(buf, os_2->usWeightClass);
	bufwrite16b(buf, os_2->usWidthClass);
	bufwrite16b(buf, os_2->fsType);
	bufwrite16b(buf, os_2->ySubscriptXSize);
	bufwrite16b(buf, os_2->ySubscriptYSize);
	bufwrite16b(buf, os_2->ySubscriptXOffset);
	bufwrite16b(buf, os_2->ySubscriptYOffset);
	bufwrite16b(buf, os_2->ySupscriptXSize);
	bufwrite16b(buf, os_2->ySupscriptYSize);
	bufwrite16b(buf, os_2->ySupscriptXOffset);
	bufwrite16b(buf, os_2->ySupscriptYOffset);
	bufwrite16b(buf, os_2->yStrikeoutSize);
	bufwrite16b(buf, os_2->yStrikeoutPosition);
	bufwrite16b(buf, os_2->sFamilyClass);
	bufwrite_bytes(buf, 10, os_2->panose);
	bufwrite32b(buf, os_2->ulUnicodeRange1);
	bufwrite32b(buf, os_2->ulUnicodeRange2);
	bufwrite32b(buf, os_2->ulUnicodeRange3);
	bufwrite32b(buf, os_2->ulUnicodeRange4);
	bufwrite_bytes(buf, 4, os_2->achVendID);
	bufwrite16b(buf, os_2->fsSelection);
	bufwrite16b(buf, os_2->usFirstCharIndex);
	bufwrite16b(buf, os_2->usLastCharIndex);
	bufwrite16b(buf, os_2->sTypoAscender);
	bufwrite16b(buf, os_2->sTypoDescender);
	bufwrite16b(buf, os_2->sTypoLineGap);
	bufwrite16b(buf, os_2->usWinAscent);
	bufwrite16b(buf, os_2->usWinDescent);
	bufwrite32b(buf, os_2->ulCodePageRange1);
	bufwrite32b(buf, os_2->ulCodePageRange2);
	if (os_2->version < 2) return buf;
	bufwrite16b(buf, os_2->sxHeight);
	bufwrite16b(buf, os_2->sCapHeight);
	bufwrite16b(buf, os_2->usDefaultChar);
	bufwrite16b(buf, os_2->usBreakChar);
	bufwrite16b(buf, os_2->usMaxContext);
	if (os_2->version < 5) return buf;
	bufwrite16b(buf, os_2->usLowerOpticalPointSize);
	bufwrite16b(buf, os_2->usUpperOpticalPointSize);
	return buf;
}
