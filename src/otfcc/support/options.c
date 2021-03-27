#include "otfcc/options.h"
#include "support/otfcc-alloc.h"

otfcc_Options *otfcc_newOptions() {
	otfcc_Options *options;
	NEW(options);
	return options;
}
void otfcc_deleteOptions(otfcc_Options *options) {
	if (options) {
		FREE(options->glyph_name_prefix);
		if (options->logger) options->logger->dispose(options->logger);
	}
	FREE(options);
}
void otfcc_Options_optimizeTo(otfcc_Options *options, uint8_t level) {
	options->cff_rollCharString = false;
	options->short_post = false;
	options->ignore_glyph_order = false;
	options->cff_short_vmtx = false;
	options->merge_features = false;
	options->force_cid = false;
	options->cff_doSubroutinize = false;

	if (level >= 1) {
		options->cff_rollCharString = true;
		options->cff_short_vmtx = true;
	}
	if (level >= 2) {
		options->short_post = true;
		options->cff_doSubroutinize = true;
		options->merge_features = true;
	}
	if (level >= 3) {
		options->ignore_glyph_order = true;
		options->force_cid = true;
	}
}
