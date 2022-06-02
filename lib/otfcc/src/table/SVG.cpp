#include "SVG.h"

#include "support/util.h"
#include "bk/bkgraph.h"
static INLINE void initSVGAssigment(svg_Assignment *a) {
	memset(a, 0, sizeof(*a));
}
static INLINE void copySVGAssigment(svg_Assignment *dst, const svg_Assignment *src) {
	dst->start = src->start;
	dst->end = src->end;
	dst->document = bufnew();
	bufwrite_buf(dst->document, src->document);
}
static INLINE void disposeSVGAssignment(svg_Assignment *a) {
	buffree(a->document);
}
caryll_standardValType(svg_Assignment, svg_iAssignment, initSVGAssigment, copySVGAssigment,
                       disposeSVGAssignment);
caryll_standardVectorImpl(table_SVG, svg_Assignment, svg_iAssignment, table_iSVG);

table_SVG *otfcc_readSVG(const otfcc_Packet packet, const otfcc_Options *options) {
	table_SVG *svg = NULL;
	FOR_TABLE('SVG ', table) {
		if (table.length < 10) goto FAIL;
		uint32_t offsetToSVGDocIndex = read_32u(table.data + 2);
		if (table.length < offsetToSVGDocIndex + 2) goto FAIL;
		uint16_t numEntries = read_16u(table.data + offsetToSVGDocIndex);
		if (table.length < offsetToSVGDocIndex + 2 + 12 * numEntries) goto FAIL;

		svg = table_iSVG.create();
		for (glyphid_t j = 0; j < numEntries; j++) {
			font_file_pointer record = table.data + offsetToSVGDocIndex + 2 + 12 * j;
			svg_Assignment asg = svg_iAssignment.empty();
			asg.start = read_16u(record);
			asg.end = read_16u(record + 2);
			uint32_t docstart = read_32u(record + 4);
			uint32_t doclen = read_32u(record + 8);
			if (offsetToSVGDocIndex + docstart + doclen <= table.length) {
				asg.document = bufnew();
				bufwrite_bytes(asg.document, doclen, table.data + offsetToSVGDocIndex + docstart);
			} else {
				asg.document = bufnew();
			}
			table_iSVG.push(svg, asg);
		}
		return svg;
	FAIL:
		table_iSVG.dispose(svg);
		svg = NULL;
	}
	return svg;
}

static bool canUsePlainFormat(const caryll_Buffer *buf) {
	return (buf->size > 4 && buf->data[0] == '<' && buf->data[1] == 's' && buf->data[2] == 'v' &&
	        buf->data[3] == 'g') // <svg
	       || (buf->size > 5 && buf->data[0] == '<' && buf->data[1] == '?' && buf->data[2] == 'x' &&
	           buf->data[3] == 'm' && buf->data[4] == 'l');
}
void otfcc_dumpSVG(const table_SVG *svg, json_value *root, const otfcc_Options *options) {
	if (!svg) return;
	loggedStep("SVG ") {
		json_value *_svg = json_array_new(svg->length);
		foreach (svg_Assignment *a, *svg) {
			json_value *_a = json_object_new(4);
			json_object_push(_a, "start", json_integer_new(a->start));
			json_object_push(_a, "end", json_integer_new(a->end));
			if (canUsePlainFormat(a->document)) {
				json_object_push(_a, "format", json_string_new("plain"));
				json_object_push(
				    _a, "document",
				    json_string_new_length((uint32_t)a->document->size, (char *)a->document->data));
			} else {
				size_t len = 0;
				uint8_t *buf = base64_encode(a->document->data, a->document->size, &len);
				json_object_push(_a, "format", json_string_new("base64"));
				json_object_push(_a, "document",
				                 json_string_new_length((uint32_t)len, (char *)buf));
				FREE(buf);
			}
			json_array_push(_svg, _a);
		}
		json_object_push(root, "SVG_", _svg);
	}
}

table_SVG *otfcc_parseSVG(const json_value *root, const otfcc_Options *options) {
	json_value *_svg = NULL;
	if (!(_svg = json_obj_get_type(root, "SVG_", json_array))) return NULL;
	table_SVG *svg = table_iSVG.create();
	loggedStep("SVG ") {
		for (glyphid_t j = 0; j < _svg->u.array.length; j++) {
			json_value *_a = _svg->u.array.values[j];
			if (!_a || _a->type != json_object) continue;
			const char *format = json_obj_getstr_share(_a, "format");
			sds doc = json_obj_getsds(_a, "document");
			if (!format || !doc) continue;
			svg_Assignment asg = svg_iAssignment.empty();
			asg.start = json_obj_getint(_a, "start");
			asg.end = json_obj_getint(_a, "end");

			if (strcmp(format, "plain") == 0) {
				asg.document = bufnew();
				bufwrite_bytes(asg.document, sdslen(doc), (uint8_t *)doc);
				sdsfree(doc);
			} else {
				asg.document = bufnew();
				size_t len = 0;
				uint8_t *buf = base64_encode((uint8_t *)doc, sdslen(doc), &len);
				bufwrite_bytes(asg.document, len, buf);
				FREE(buf);
				sdsfree(doc);
			}
			table_iSVG.push(svg, asg);
		}
	}
	return svg;
}

static int byStartGID(const svg_Assignment *a, const svg_Assignment *b) {
	return a->start - b->start;
}

caryll_Buffer *otfcc_buildSVG(const table_SVG *_svg, const otfcc_Options *options) {
	if (!_svg || !_svg->length) return NULL;

	// sort assignments
	table_SVG svg;
	table_iSVG.copy(&svg, _svg);
	table_iSVG.sort(&svg, byStartGID);

	bk_Block *major = bk_new_Block(b16, svg.length, // numEntries
	                               bkover);
	foreach (svg_Assignment *a, svg) {
		bk_push(major,                                       // SVG Document Index Entry
		        b16, a->start,                               // startGlyphID
		        b16, a->end,                                 // endGlyphID
		        p32, bk_newBlockFromBufferCopy(a->document), // svgDocOffset
		        b32, a->document->size,                      // svgDocLength
		        bkover);
	}
	bk_Block *root = bk_new_Block(b16, 0,     // Version
	                              p32, major, // offsetToSVGDocIndex
	                              b32, 0,     // Reserved
	                              bkover);
	table_iSVG.dispose(&svg);
	return bk_build_Block(root);
}
