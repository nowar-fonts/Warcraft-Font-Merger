#include "dep/json-builder.h"
#include "otfcc/sfnt.h"
#include "otfcc/font.h"

#include "aliases.h"
#include "platform.h"
#include "stopwatch.h"

// - MODIFIED FOR WFM
#include <config.h>
void printInfo() {
	fprintf(stdout, "This is WFM otfccdump, version %d.%d.%d.\n",
	        WFM_all_VERSION_MAJOR,
	        WFM_all_VERSION_MINOR,
	        WFM_all_VERSION_PATCH);
}
void printHelp() {
	fprintf(stdout,
	        "\n"
	        "Usage : otfccdump [OPTIONS] input.[otf|ttf|ttc]\n\n"
	        " -h, --help              : Display this help message and exit.\n"
	        " -v, --version           : Display version information and exit.\n"
	        " -o <file>               : Set output file path to <file>. When absent the dump\n"
	        "                           will be written to STDOUT.\n"
	        " -n <n>, --ttc-index <n> : Use the <n>th subfont within the input font.\n"
	        " --pretty                : Prettify the output JSON.\n"
	        " --ugly                  : Force uglify the output JSON.\n"
	        " --verbose               : Show more information when building.\n"
	        " -q, --quiet             : Be silent when building.\n\n"
	        " --ignore-glyph-order    : Do not export glyph order information.\n"
	        " --glyph-name-prefix pfx : Add a prefix to the glyph names.\n"
	        " --ignore-hints          : Do not export hinting information.\n"
	        " --decimal-cmap          : Export 'cmap' keys as decimal number.\n"
	        " --hex-cmap              : Export 'cmap' keys as hex number (U+FFFF).\n"
	        " --name-by-hash          : Name glyphs using its hash value.\n"
	        " --name-by-gid           : Name glyphs using its glyph id.\n"
	        " --add-bom               : Add BOM mark in the output. (It is default on Windows\n"
	        "                           when redirecting to another program. Use --no-bom to\n"
	        "                           turn it off.)\n"
	        "\n");
}
#ifdef _WIN32
int main() {
	int argc;
	char **argv;
	get_argv_utf8(&argc, &argv);
#else
int main(int argc, char *argv[]) {
#endif

	bool show_help = false;
	bool show_version = false;
	bool show_pretty = false;
	bool show_ugly = false;
	bool add_bom = false;
	bool no_bom = false;
	uint32_t ttcindex = 0;
	struct option longopts[] = {{"version", no_argument, NULL, 'v'},
	                            {"help", no_argument, NULL, 'h'},
	                            {"pretty", no_argument, NULL, 'p'},
	                            {"ugly", no_argument, NULL, 0},
	                            {"time", no_argument, NULL, 0},
	                            {"ignore-glyph-order", no_argument, NULL, 0},
	                            {"ignore-hints", no_argument, NULL, 0},
	                            {"hex-cmap", no_argument, NULL, 0},
	                            {"decimal-cmap", no_argument, NULL, 0},
	                            {"instr-as-bytes", no_argument, NULL, 0},
	                            {"name-by-hash", no_argument, NULL, 0},
	                            {"name-by-gid", no_argument, NULL, 0},
	                            {"glyph-name-prefix", required_argument, NULL, 0},
	                            {"verbose", no_argument, NULL, 0},
	                            {"quiet", no_argument, NULL, 0},
	                            {"add-bom", no_argument, NULL, 0},
	                            {"no-bom", no_argument, NULL, 0},
	                            {"output", required_argument, NULL, 'o'},
	                            {"ttc-index", required_argument, NULL, 'n'},
	                            {"debug-wait-on-start", no_argument, NULL, 0},
	                            {0, 0, 0, 0}};

	otfcc_Options *options = otfcc_newOptions();
	options->logger = otfcc_newLogger(otfcc_newStdErrTarget());
	options->logger->indent(options->logger, "otfccdump");
	options->decimal_cmap = true;

	int option_index = 0;
	int c;

	sds outputPath = NULL;
	sds inPath = NULL;

	while ((c = getopt_long(argc, argv, "vhqpio:n:", longopts, &option_index)) != (-1)) {
		switch (c) {
			case 0:
				/* If this option set a flag, do nothing else now. */
				if (longopts[option_index].flag != 0) {
					break;
				} else if (strcmp(longopts[option_index].name, "ugly") == 0) {
					show_ugly = true;
				} else if (strcmp(longopts[option_index].name, "time") == 0) {
				} else if (strcmp(longopts[option_index].name, "add-bom") == 0) {
					add_bom = true;
				} else if (strcmp(longopts[option_index].name, "no-bom") == 0) {
					no_bom = true;
				} else if (strcmp(longopts[option_index].name, "ignore-glyph-order") == 0) {
					options->ignore_glyph_order = true;
				} else if (strcmp(longopts[option_index].name, "verbose") == 0) {
					options->verbose = true;
				} else if (strcmp(longopts[option_index].name, "quiet") == 0) {
					options->quiet = true;
				} else if (strcmp(longopts[option_index].name, "ignore-hints") == 0) {
					options->ignore_hints = true;
				} else if (strcmp(longopts[option_index].name, "decimal-cmap") == 0) {
					options->decimal_cmap = true;
				} else if (strcmp(longopts[option_index].name, "hex-cmap") == 0) {
					options->decimal_cmap = false;
				} else if (strcmp(longopts[option_index].name, "name-by-hash") == 0) {
					options->name_glyphs_by_hash = true;
				} else if (strcmp(longopts[option_index].name, "name-by-gid") == 0) {
					options->name_glyphs_by_gid = true;
				} else if (strcmp(longopts[option_index].name, "instr-as-bytes") == 0) {
					options->instr_as_bytes = true;
				} else if (strcmp(longopts[option_index].name, "glyph-name-prefix") == 0) {
					options->glyph_name_prefix = strdup(optarg);
				} else if (strcmp(longopts[option_index].name, "debug-wait-on-start") == 0) {
					options->debug_wait_on_start = true;
				}
				break;
			case 'v':
				show_version = true;
				break;
			case 'i':
				options->ignore_glyph_order = true;
				break;
			case 'h':
				show_help = true;
				break;
			case 'p':
				show_pretty = true;
				break;
			case 'o':
				outputPath = sdsnew(optarg);
				break;
			case 'q':
				options->quiet = true;
				break;
			case 'n':
				ttcindex = atoi(optarg);
				break;
		}
	}

	if (options->debug_wait_on_start) { getchar(); }

	options->logger->setVerbosity(options->logger,
	                              options->quiet ? 0 : options->verbose ? 0xFF : 1);

	if (show_help) {
		printInfo();
		printHelp();
		return 0;
	}
	if (show_version) {
		printInfo();
		return 0;
	}

	if (optind >= argc) {
		logError("Expected argument for input file name.\n");
		printHelp();
		exit(EXIT_FAILURE);
	} else {
		inPath = sdsnew(argv[optind]);
	}

	struct timespec begin;

	time_now(&begin);

	otfcc_SplineFontContainer *sfnt;
	loggedStep("Read SFNT") {
		logProgress("From file %s", inPath);
		FILE *file = u8fopen(inPath, "rb");
		sfnt = otfcc_readSFNT(file);
		if (!sfnt || sfnt->count == 0) {
			logError("Cannot read SFNT file \"%s\". Exit.\n", inPath);
			exit(EXIT_FAILURE);
		}
		if (ttcindex >= sfnt->count) {
			logError("Subfont index %d out of range for \"%s\" (0 -- %d). Exit.\n", ttcindex,
			         inPath, (sfnt->count - 1));
			exit(EXIT_FAILURE);
		}
		logStepTime;
	}

	otfcc_Font *font;
	loggedStep("Read Font") {
		otfcc_IFontBuilder *reader = otfcc_newOTFReader();
		font = reader->read(sfnt, ttcindex, options);
		if (!font) {
			logError("Font structure broken or corrupted \"%s\". Exit.\n", inPath);
			exit(EXIT_FAILURE);
		}
		reader->free(reader);
		if (sfnt) otfcc_deleteSFNT(sfnt);
		logStepTime;
	}
	loggedStep("Consolidate") {
		otfcc_iFont.consolidate(font, options);
		logStepTime;
	}
	json_value *root;
	loggedStep("Dump") {
		otfcc_IFontSerializer *dumper = otfcc_newJsonWriter();
		root = (json_value *)dumper->serialize(font, options);
		if (!root) {
			logError("Font structure broken or corrupted \"%s\". Exit.\n", inPath);
			exit(EXIT_FAILURE);
		}
		logStepTime;
		dumper->free(dumper);
	}

	char *buf;
	size_t buflen;
	loggedStep("Serialize to JSON") {
		json_serialize_opts jsonOptions;
		jsonOptions.mode = json_serialize_mode_packed;
		jsonOptions.opts = 0;
		jsonOptions.indent_size = 4;
		if (show_pretty || (!outputPath && isatty(fileno(stdout)))) {
			jsonOptions.mode = json_serialize_mode_multiline;
		}
		if (show_ugly) jsonOptions.mode = json_serialize_mode_packed;
		buflen = json_measure_ex(root, jsonOptions);
		buf = calloc(1, buflen);
		json_serialize_ex(buf, root, jsonOptions);
		logStepTime;
	}

	loggedStep("Output") {
		if (outputPath) {
			FILE *outputFile = u8fopen(outputPath, "wb");
			if (!outputFile) {
				logError("Cannot write to file \"%s\". Exit.", outputPath);
				exit(EXIT_FAILURE);
			}
			if (add_bom) {
				fputc(0xEF, outputFile);
				fputc(0xBB, outputFile);
				fputc(0xBF, outputFile);
			}
			size_t actualLen = buflen - 1;
			while (!buf[actualLen])
				actualLen -= 1;
			fwrite(buf, sizeof(char), actualLen + 1, outputFile);
			fclose(outputFile);
		} else {
#ifdef WIN32
			if (isatty(fileno(stdout))) {
				LPWSTR pwStr;
				DWORD dwNum = widen_utf8(buf, &pwStr);
				DWORD actual = 0;
				DWORD written = 0;
				const DWORD chunk = 0x10000;
				while (written < dwNum) {
					DWORD len = dwNum - written;
					if (len > chunk) len = chunk;
					WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), pwStr + written, len, &actual,
					              NULL);
					written += len;
				}
				free(pwStr);
			} else {
				if (!no_bom) {
					fputc(0xEF, stdout);
					fputc(0xBB, stdout);
					fputc(0xBF, stdout);
				}
				fputs(buf, stdout);
			}
#else
			if (add_bom) {
				fputc(0xEF, stdout);
				fputc(0xBB, stdout);
				fputc(0xBF, stdout);
			}
			fputs(buf, stdout);
#endif
		}
		logStepTime;
	}

	loggedStep("Finalize") {
		free(buf);
		if (font) otfcc_iFont.free(font);
		if (root) json_builder_free(root);
		if (inPath) sdsfree(inPath);
		if (outputPath) sdsfree(outputPath);
		logStepTime;
	}
	otfcc_deleteOptions(options);

	return 0;
}
