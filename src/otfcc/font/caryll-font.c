#include "support/util.h"
#include "otfcc/font.h"
#include "table/all.h"
#include "otfcc/sfnt-builder.h"
#include "consolidate/consolidate.h"

static void *createFontTable(otfcc_Font *font, const uint32_t tag) {
	switch (tag) {
		case 'name':
			return table_iName.create();
		case 'GSUB':
		case 'GPOS':
			return table_iOTL.create();
		default:
			return NULL;
	}
}

static void deleteFontTable(otfcc_Font *font, const uint32_t tag) {
	switch (tag) {
		case 'head':
			if (font->head) DELETE(table_iHead.free, font->head);
			return;
		case 'hhea':
			if (font->hhea) DELETE(table_iHhea.free, font->hhea);
			return;
		case 'maxp':
			if (font->maxp) DELETE(table_iMaxp.free, font->maxp);
			return;
		case 'OS_2':
		case 'OS/2':
			if (font->OS_2) DELETE(table_iOS_2.free, font->OS_2);
			return;
		case 'name':
			if (font->name) DELETE(table_iName.free, font->name);
			return;
		case 'meta':
			if (font->meta) DELETE(table_iMeta.free, font->meta);
			return;
		case 'hmtx':
			if (font->hmtx) DELETE(table_iHmtx.free, font->hmtx);
			return;
		case 'vmtx':
			if (font->vmtx) DELETE(table_iVmtx.free, font->vmtx);
			return;
		case 'post':
			if (font->post) DELETE(iTable_post.free, font->post);
			return;
#if 0
		case 'hdmx':
			if (font->hdmx) DELETE(otfcc_deleteHdmx, font->hdmx);
			return;
#endif
		case 'vhea':
			if (font->vhea) DELETE(table_iVhea.free, font->vhea);
			return;
		case 'fpgm':
			if (font->fpgm) DELETE(table_iFpgm_prep.free, font->fpgm);
			return;
		case 'prep':
			if (font->prep) DELETE(table_iFpgm_prep.free, font->prep);
			return;
		case 'cvt_':
		case 'cvt ':
			if (font->cvt_) DELETE(table_iCvt.free, font->cvt_);
			return;
		case 'gasp':
			if (font->gasp) DELETE(table_iGasp.free, font->gasp);
			return;
		case 'CFF_':
		case 'CFF ':
			if (font->CFF_) DELETE(table_iCFF.free, font->CFF_);
			return;
		case 'glyf':
			if (font->glyf) DELETE(table_iGlyf.free, font->glyf);
			return;
		case 'cmap':
			if (font->cmap) DELETE(table_iCmap.free, font->cmap);
			return;
		case 'LTSH':
			if (font->LTSH) DELETE(table_iLTSH.free, font->LTSH);
			return;
		case 'GSUB':
			if (font->GSUB) DELETE(table_iOTL.free, font->GSUB);
			return;
		case 'GPOS':
			if (font->GPOS) DELETE(table_iOTL.free, font->GPOS);
			return;
		case 'GDEF':
			if (font->GDEF) DELETE(table_iGDEF.free, font->GDEF);
			return;
		case 'BASE':
			if (font->BASE) DELETE(table_iBASE.free, font->BASE);
			return;
		case 'VORG':
			if (font->VORG) DELETE(table_iVORG.free, font->VORG);
			return;
		case 'CPAL':
			if (font->CPAL) DELETE(table_iCPAL.free, font->CPAL);
			return;
		case 'COLR':
			if (font->COLR) DELETE(table_iCOLR.free, font->COLR);
			return;
		case 'SVG ':
		case 'SVG_':
			if (font->SVG_) DELETE(table_iSVG.free, font->SVG_);
			return;
		case 'TSI0':
		case 'TSI1':
			if (font->TSI_01) DELETE(table_iTSI.free, font->TSI_01);
			return;
		case 'TSI2':
		case 'TSI3':
			if (font->TSI_23) DELETE(table_iTSI.free, font->TSI_23);
			return;
		case 'TSI5':
			if (font->TSI5) DELETE(otl_iClassDef.free, font->TSI5);
			return;
	}
}

static INLINE void initFont(otfcc_Font *font) {
	memset(font, 0, sizeof(*font));
}
static INLINE void disposeFont(otfcc_Font *font) {
	deleteFontTable(font, 'head');
	deleteFontTable(font, 'hhea');
	deleteFontTable(font, 'maxp');
	deleteFontTable(font, 'OS_2');
	deleteFontTable(font, 'name');
	deleteFontTable(font, 'meta');
	deleteFontTable(font, 'hmtx');
	deleteFontTable(font, 'vmtx');
	deleteFontTable(font, 'post');
	deleteFontTable(font, 'hdmx');
	deleteFontTable(font, 'vhea');
	deleteFontTable(font, 'fpgm');
	deleteFontTable(font, 'prep');
	deleteFontTable(font, 'cvt_');
	deleteFontTable(font, 'gasp');
	deleteFontTable(font, 'CFF_');
	deleteFontTable(font, 'glyf');
	deleteFontTable(font, 'cmap');
	deleteFontTable(font, 'LTSH');
	deleteFontTable(font, 'GSUB');
	deleteFontTable(font, 'GPOS');
	deleteFontTable(font, 'GDEF');
	deleteFontTable(font, 'BASE');
	deleteFontTable(font, 'VORG');
	deleteFontTable(font, 'CPAL');
	deleteFontTable(font, 'COLR');
	deleteFontTable(font, 'SVG_');
	deleteFontTable(font, 'TSI0');
	deleteFontTable(font, 'TSI2');
	deleteFontTable(font, 'TSI5');

	GlyphOrder.free(font->glyph_order);
}
caryll_standardRefTypeFn(otfcc_Font, initFont, disposeFont);

caryll_ElementInterfaceOf(otfcc_Font) otfcc_iFont = {
    caryll_standardRefTypeMethods(otfcc_Font),
    .createTable = createFontTable,
    .deleteTable = deleteFontTable,
    .consolidate = otfcc_consolidateFont,
};
