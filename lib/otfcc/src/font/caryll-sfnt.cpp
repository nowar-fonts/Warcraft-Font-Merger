#include "support/util.h"
#include "otfcc/sfnt.h"

static void otfcc_read_packets(otfcc_SplineFontContainer *font, FILE *file) {
	for (uint32_t count = 0; count < font->count; count++) {
		(void)fseek(file, font->offsets[count], SEEK_SET);

		font->packets[count].sfnt_version = otfcc_get32u(file);
		font->packets[count].numTables = otfcc_get16u(file);
		font->packets[count].searchRange = otfcc_get16u(file);
		font->packets[count].entrySelector = otfcc_get16u(file);
		font->packets[count].rangeShift = otfcc_get16u(file);
		NEW(font->packets[count].pieces, font->packets[count].numTables);

		for (uint32_t i = 0; i < font->packets[count].numTables; i++) {
			font->packets[count].pieces[i].tag = otfcc_get32u(file);
			font->packets[count].pieces[i].checkSum = otfcc_get32u(file);
			font->packets[count].pieces[i].offset = otfcc_get32u(file);
			font->packets[count].pieces[i].length = otfcc_get32u(file);
			NEW(font->packets[count].pieces[i].data, font->packets[count].pieces[i].length);
		}

		for (uint32_t i = 0; i < font->packets[0].numTables; i++) {
			(void)fseek(file, font->packets[count].pieces[i].offset, SEEK_SET);
			(void)fread(font->packets[count].pieces[i].data, font->packets[count].pieces[i].length,
			            1, file);
		}
	}
}

otfcc_SplineFontContainer *otfcc_readSFNT(FILE *file) {
	if (!file) return NULL;
	otfcc_SplineFontContainer *font;
	NEW(font);

	font->type = otfcc_get32u(file);

	switch (font->type) {
		case 'OTTO':
		case 0x00010000:
		case 'true':
		case 'typ1':
			font->count = 1;
			NEW(font->offsets, font->count);
			NEW(font->packets, font->count);
			font->offsets[0] = 0;
			otfcc_read_packets(font, file);
			break;

		case 'ttcf':
			(void)otfcc_get32u(file);
			font->count = otfcc_get32u(file);
			NEW(font->offsets, font->count);
			NEW(font->packets, font->count);

			for (uint32_t i = 0; i < font->count; i++) {
				font->offsets[i] = otfcc_get32u(file);
			}

			otfcc_read_packets(font, file);
			break;

		default:
			font->count = 0;
			font->offsets = NULL;
			font->packets = NULL;
			break;
	}

	fclose(file);

	return font;
}

void otfcc_deleteSFNT(otfcc_SplineFontContainer *font) {
	if (!font) return;
	if (font->count > 0) {
		for (uint32_t count = 0; count < font->count; count++) {
			for (int i = 0; i < font->packets[count].numTables; i++) {
				FREE(font->packets[count].pieces[i].data);
			}
			FREE(font->packets[count].pieces);
		}
		FREE(font->packets);
	}
	FREE(font->offsets);
	FREE(font);
}
