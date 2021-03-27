#include "support/util.h"
#include "otfcc/font.h"
#include "table/all.h"

#include "unconsolidate.h"

static otfcc_font_subtype decideFontSubtypeOTF(otfcc_SplineFontContainer *sfnt, uint32_t index) {
	otfcc_Packet packet = sfnt->packets[index];
	FOR_TABLE_SILENT('CFF ', table) {
		return FONTTYPE_CFF;
	}
	return FONTTYPE_TTF;
}

static otfcc_Font *readOtf(void *_sfnt, uint32_t index, const otfcc_Options *options) {
	otfcc_SplineFontContainer *sfnt = (otfcc_SplineFontContainer *)_sfnt;
	if (sfnt->count - 1 < index) {
		return NULL;
	} else {
		otfcc_Font *font = otfcc_iFont.create();
		otfcc_Packet packet = sfnt->packets[index];
		font->subtype = decideFontSubtypeOTF(sfnt, index);
		font->fvar = otfcc_readFvar(packet, options);
		font->head = otfcc_readHead(packet, options);
		font->maxp = otfcc_readMaxp(packet, options);
		font->name = otfcc_readName(packet, options);
		font->meta = otfcc_readMeta(packet, options);
		font->OS_2 = otfcc_readOS_2(packet, options);
		font->post = otfcc_readPost(packet, options);
		font->hhea = otfcc_readHhea(packet, options);
		font->cmap = otfcc_readCmap(packet, options);
		if (font->subtype == FONTTYPE_TTF) {
			font->hmtx = otfcc_readHmtx(packet, options, font->hhea, font->maxp);
			font->vhea = otfcc_readVhea(packet, options);
			if (font->vhea) font->vmtx = otfcc_readVmtx(packet, options, font->vhea, font->maxp);
			font->fpgm = otfcc_readFpgmPrep(packet, options, 'fpgm');
			font->prep = otfcc_readFpgmPrep(packet, options, 'prep');
			font->cvt_ = otfcc_readCvt(packet, options, 'cvt ');
			font->gasp = otfcc_readGasp(packet, options);
			font->VDMX = otfcc_readVDMX(packet, options);
			font->LTSH = otfcc_readLTSH(packet, options);

			GlyfIOContext ctx = {.locaIsLong = font->head->indexToLocFormat,
			                     .numGlyphs = font->maxp->numGlyphs,
			                     .nPhantomPoints = 4, // Since MS rasterizer v1.7,
			                                          // it would always add 4 phantom points
			                     .fvar = font->fvar};
			font->glyf = otfcc_readGlyf(packet, options, &ctx);
		} else {
			table_CFFAndGlyf cffpr = otfcc_readCFFAndGlyfTables(packet, options, font->head);
			font->CFF_ = cffpr.meta;
			font->glyf = cffpr.glyphs;
			font->vhea = otfcc_readVhea(packet, options);
			if (font->vhea) {
				font->vmtx = otfcc_readVmtx(packet, options, font->vhea, font->maxp);
				font->VORG = otfcc_readVORG(packet, options);
			}
		}
		if (font->glyf) {
			font->GSUB = otfcc_readOtl(packet, options, 'GSUB', font->glyf->length);
			font->GPOS = otfcc_readOtl(packet, options, 'GPOS', font->glyf->length);
			font->GDEF = otfcc_readGDEF(packet, options);
		}
		font->BASE = otfcc_readBASE(packet, options);

		// Color font
		font->CPAL = otfcc_readCPAL(packet, options);
		font->COLR = otfcc_readCOLR(packet, options);
		font->SVG_ = otfcc_readSVG(packet, options);

		// VTT TSI entries
		font->TSI_01 = otfcc_readTSI(packet, options, 'TSI0', 'TSI1');
		font->TSI_23 = otfcc_readTSI(packet, options, 'TSI2', 'TSI3');
		font->TSI5 = otfcc_readTSI5(packet, options);

		otfcc_unconsolidateFont(font, options);
		return font;
	}
}
static INLINE void freeReader(otfcc_IFontBuilder *self) {
	free(self);
}
otfcc_IFontBuilder *otfcc_newOTFReader() {
	otfcc_IFontBuilder *reader;
	NEW(reader);
	reader->read = readOtf;
	reader->free = freeReader;
	return reader;
}
