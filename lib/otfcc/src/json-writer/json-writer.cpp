#include "support/util.h"
#include "otfcc/font.h"
#include "table/all.h"

static void *serializeToJson(otfcc_Font *font, const otfcc_Options *options) {
	json_value *root = json_object_new(48);
	if (!root) return NULL;
	otfcc_dumpFvar(font->fvar, root, options);
	otfcc_dumpHead(font->head, root, options);
	otfcc_dumpHhea(font->hhea, root, options);
	otfcc_dumpMaxp(font->maxp, root, options);
	otfcc_dumpVhea(font->vhea, root, options);
	otfcc_dumpPost(font->post, root, options);
	otfcc_dumpOS_2(font->OS_2, root, options);
	otfcc_dumpName(font->name, root, options);
	otfcc_dumpMeta(font->meta, root, options);
	otfcc_dumpCmap(font->cmap, root, options);
	otfcc_dumpCFF(font->CFF_, root, options);

	GlyfIOContext ctx = {.locaIsLong = font->head->indexToLocFormat,
	                     .numGlyphs = font->maxp->numGlyphs,
	                     .nPhantomPoints = 4,
	                     .hasVerticalMetrics = !!(font->vhea),
	                     .exportFDSelect = font->CFF_ && font->CFF_->isCID,
	                     .fvar = font->fvar};
	otfcc_dumpGlyf(font->glyf, root, options, &ctx);
	if (!options->ignore_hints) {
		table_dumpTableFpgmPrep(font->fpgm, root, options, "fpgm");
		table_dumpTableFpgmPrep(font->prep, root, options, "prep");
		otfcc_dumpCvt(font->cvt_, root, options, "cvt_");
		otfcc_dumpGasp(font->gasp, root, options);
	}
	otfcc_dumpVDMX(font->VDMX, root, options);
	otfcc_dumpOtl(font->GSUB, root, options, "GSUB");
	otfcc_dumpOtl(font->GPOS, root, options, "GPOS");
	otfcc_dumpGDEF(font->GDEF, root, options);
	otfcc_dumpBASE(font->BASE, root, options);

	otfcc_dumpCPAL(font->CPAL, root, options);
	otfcc_dumpCOLR(font->COLR, root, options);
	otfcc_dumpSVG(font->SVG_, root, options);
	otfcc_dumpTSI(font->TSI_01, root, options, "TSI_01");
	otfcc_dumpTSI(font->TSI_23, root, options, "TSI_23");
	otfcc_dumpTSI5(font->TSI5, root, options);
	return root;
}
static void freeJsonWriter(otfcc_IFontSerializer *self) {
	free(self);
}
otfcc_IFontSerializer *otfcc_newJsonWriter() {
	otfcc_IFontSerializer *writer;
	NEW(writer);
	writer->serialize = serializeToJson;
	writer->free = freeJsonWriter;
	return writer;
}
