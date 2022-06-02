#include "otfcc/sfnt.h"
#include "otfcc/font.h"
#include "otfcc/sfnt-builder.h"

#ifdef _WIN32
#define OTFCC_DLL_EXPORT __declspec(dllexport)
#else
#define OTFCC_DLL_EXPORT
#endif

OTFCC_DLL_EXPORT caryll_Buffer *otfccbuild_json_otf(uint32_t inlen, const char *injson, uint8_t olevel,
                                                    bool for_webfont) {
	otfcc_Options *options = otfcc_newOptions();
	options->logger = otfcc_newLogger(otfcc_newEmptyTarget());
	options->logger->indent(options->logger, "otfccbuild");

	// optimization levels
	otfcc_Options_optimizeTo(options, olevel);
	if (for_webfont) {
		options->ignore_glyph_order = true;
		options->force_cid = true;
	}

	// json parsing
	json_value *jsonRoot = json_parse(injson, inlen);
	if (!jsonRoot) { return NULL; }
	// font parsing
	otfcc_IFontBuilder *parser = otfcc_newJsonReader();
	otfcc_Font *font = parser->read(jsonRoot, 0, options);
	parser->free(parser);
	json_value_free(jsonRoot);
	if (!font) { return NULL; }

	// consolidation and build
	otfcc_iFont.consolidate(font, options);
	otfcc_IFontSerializer *writer = otfcc_newOTFWriter();
	caryll_Buffer *otf = (caryll_Buffer *)writer->serialize(font, options);

	writer->free(writer);
	otfcc_iFont.free(font);
	return otf;
}

OTFCC_DLL_EXPORT size_t otfcc_get_buf_len(caryll_Buffer *buf) {
	return buf->size;
}
OTFCC_DLL_EXPORT uint8_t *otfcc_get_buf_data(caryll_Buffer *buf) {
	return buf->data;
}
OTFCC_DLL_EXPORT void otfccbuild_free_otfbuf(caryll_Buffer *buf) {
	buffree(buf);
}
