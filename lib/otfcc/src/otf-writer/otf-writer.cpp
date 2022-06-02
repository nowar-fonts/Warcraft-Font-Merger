#include "support/util.h"
#include "otfcc/font.h"
#include "table/all.h"
#include "otfcc/sfnt-builder.h"
#include "stat.h"

static void *serializeToOTF(otfcc_Font *font, const otfcc_Options *options) {
	// do stat before serialize
	otfcc_statFont(font, options);

	otfcc_SFNTBuilder *builder =
	    otfcc_newSFNTBuilder(font->subtype == FONTTYPE_CFF ? 'OTTO' : 0x00010000, options);
	// Outline data
	if (font->subtype == FONTTYPE_TTF) {
		table_GlyfAndLocaBuffers pair = otfcc_buildGlyf(font->glyf, font->head, options);
		otfcc_SFNTBuilder_pushTable(builder, 'glyf', pair.glyf);
		otfcc_SFNTBuilder_pushTable(builder, 'loca', pair.loca);
	} else {
		table_CFFAndGlyf r = {font->CFF_, font->glyf};
		otfcc_SFNTBuilder_pushTable(builder, 'CFF ', otfcc_buildCFF(r, options));
	}

	otfcc_SFNTBuilder_pushTable(builder, 'head', otfcc_buildHead(font->head, options));
	otfcc_SFNTBuilder_pushTable(builder, 'hhea', otfcc_buildHhea(font->hhea, options));
	otfcc_SFNTBuilder_pushTable(builder, 'OS/2', otfcc_buildOS_2(font->OS_2, options));
	otfcc_SFNTBuilder_pushTable(builder, 'maxp', otfcc_buildMaxp(font->maxp, options));
	otfcc_SFNTBuilder_pushTable(builder, 'name', otfcc_buildName(font->name, options));
	otfcc_SFNTBuilder_pushTable(builder, 'meta', otfcc_buildMeta(font->meta, options));
	otfcc_SFNTBuilder_pushTable(builder, 'post',
	                            otfcc_buildPost(font->post, font->glyph_order, options));
	otfcc_SFNTBuilder_pushTable(builder, 'cmap', otfcc_buildCmap(font->cmap, options));
	otfcc_SFNTBuilder_pushTable(builder, 'gasp', otfcc_buildGasp(font->gasp, options));

	if (font->subtype == FONTTYPE_TTF) {
		otfcc_SFNTBuilder_pushTable(builder, 'fpgm', otfcc_buildFpgmPrep(font->fpgm, options));
		otfcc_SFNTBuilder_pushTable(builder, 'prep', otfcc_buildFpgmPrep(font->prep, options));
		otfcc_SFNTBuilder_pushTable(builder, 'cvt ', otfcc_buildCvt(font->cvt_, options));
		otfcc_SFNTBuilder_pushTable(builder, 'LTSH', otfcc_buildLTSH(font->LTSH, options));
		otfcc_SFNTBuilder_pushTable(builder, 'VDMX', otfcc_buildVDMX(font->VDMX, options));
	}

	if (font->hhea && font->maxp && font->hmtx) {
		uint16_t hmtx_counta = font->hhea->numberOfMetrics;
		uint16_t hmtx_countk = font->maxp->numGlyphs - font->hhea->numberOfMetrics;
		otfcc_SFNTBuilder_pushTable(builder, 'hmtx',
		                            otfcc_buildHmtx(font->hmtx, hmtx_counta, hmtx_countk, options));
	}

	otfcc_SFNTBuilder_pushTable(builder, 'vhea', otfcc_buildVhea(font->vhea, options));
	if (font->vhea && font->maxp && font->vmtx) {
		uint16_t vmtx_counta = font->vhea->numOfLongVerMetrics;
		uint16_t vmtx_countk = font->maxp->numGlyphs - font->vhea->numOfLongVerMetrics;
		otfcc_SFNTBuilder_pushTable(builder, 'vmtx',
		                            otfcc_buildVmtx(font->vmtx, vmtx_counta, vmtx_countk, options));
	}
	otfcc_SFNTBuilder_pushTable(builder, 'VORG', otfcc_buildVORG(font->VORG, options));

	otfcc_SFNTBuilder_pushTable(builder, 'GSUB', otfcc_buildOtl(font->GSUB, options, "GSUB"));
	otfcc_SFNTBuilder_pushTable(builder, 'GPOS', otfcc_buildOtl(font->GPOS, options, "GPOS"));
	otfcc_SFNTBuilder_pushTable(builder, 'GDEF', otfcc_buildGDEF(font->GDEF, options));
	otfcc_SFNTBuilder_pushTable(builder, 'BASE', otfcc_buildBASE(font->BASE, options));

	otfcc_SFNTBuilder_pushTable(builder, 'CPAL', otfcc_buildCPAL(font->CPAL, options));
	otfcc_SFNTBuilder_pushTable(builder, 'COLR', otfcc_buildCOLR(font->COLR, options));
	otfcc_SFNTBuilder_pushTable(builder, 'SVG ', otfcc_buildSVG(font->SVG_, options));

	{
		tsi_BuildTarget target = otfcc_buildTSI(font->TSI_01, options);
		otfcc_SFNTBuilder_pushTable(builder, 'TSI0', target.indexPart);
		otfcc_SFNTBuilder_pushTable(builder, 'TSI1', target.textPart);
	}
	{
		tsi_BuildTarget target = otfcc_buildTSI(font->TSI_23, options);
		otfcc_SFNTBuilder_pushTable(builder, 'TSI2', target.indexPart);
		otfcc_SFNTBuilder_pushTable(builder, 'TSI3', target.textPart);
	}
	if (font->glyf) {
		otfcc_SFNTBuilder_pushTable(builder, 'TSI5',
		                            otfcc_buildTSI5(font->TSI5, options, font->glyf->length));
	}

	if (options->dummy_DSIG) {
		caryll_Buffer *dsig = bufnew();
		bufwrite32b(dsig, 0x00000001);
		bufwrite16b(dsig, 0);
		bufwrite16b(dsig, 0);
		otfcc_SFNTBuilder_pushTable(builder, 'DSIG', dsig);
	}

	caryll_Buffer *otf = otfcc_SFNTBuilder_serialize(builder);
	otfcc_deleteSFNTBuilder(builder);
	otfcc_unstatFont(font, options);
	return otf;
}
static void freeFontWriter(otfcc_IFontSerializer *self) {
	free(self);
}
otfcc_IFontSerializer *otfcc_newOTFWriter() {
	otfcc_IFontSerializer *writer;
	NEW(writer);
	writer->serialize = serializeToOTF;
	writer->free = freeFontWriter;
	return writer;
}
