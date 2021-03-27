#include "cff-charset.h"

void cff_extract_Charset(uint8_t *data, int32_t offset, uint16_t nchars, cff_Charset *charsets) {
	uint32_t i;
	if (offset == cff_CHARSET_ISOADOBE)
		charsets->t = cff_CHARSET_ISOADOBE;
	else if (offset == cff_CHARSET_EXPERT)
		charsets->t = cff_CHARSET_EXPERT;
	else if (offset == cff_CHARSET_EXPERTSUBSET)
		charsets->t = cff_CHARSET_EXPERTSUBSET;
	else {
		// NOTE: gid 1 will always be named as .notdef
		switch (data[offset]) {
			case 0:
				charsets->t = cff_CHARSET_FORMAT0;
				{
					charsets->s = nchars - 1;
					NEW(charsets->f0.glyph, nchars - 1);

					for (i = 0; i < charsets->s; i++)
						charsets->f0.glyph[i] = gu2(data, offset + 1 + i * 2);
				}
				break;
			case 1:
				charsets->t = cff_CHARSET_FORMAT1;
				{
					uint32_t size;
					uint32_t glyphsEncodedSofar = 1;
					for (i = 0; glyphsEncodedSofar < nchars; i++) {
						glyphsEncodedSofar += 1 + gu1(data, offset + 3 + i * 3);
					}

					size = i;
					charsets->s = size;
					NEW(charsets->f1.range1, i + 1);
					for (i = 0; i < size; i++) {
						charsets->f1.range1[i].first = gu2(data, offset + 1 + i * 3);
						charsets->f1.range1[i].nleft = gu1(data, offset + 3 + i * 3);
					}
				}
				break;
			case 2:
				charsets->t = cff_CHARSET_FORMAT2;
				{
					uint32_t size;
					uint32_t glyphsEncodedSofar = 1;
					for (i = 0; glyphsEncodedSofar < nchars; i++) {
						glyphsEncodedSofar += 1 + gu2(data, offset + 3 + i * 4);
					}

					size = i;
					charsets->s = size;
					NEW(charsets->f2.range2, i + 1);

					for (i = 0; i < size; i++) {
						charsets->f2.range2[i].first = gu2(data, offset + 1 + i * 4);
						charsets->f2.range2[i].nleft = gu2(data, offset + 3 + i * 4);
					}
				}
				break;
		}
	}
}

caryll_Buffer *cff_build_Charset(cff_Charset cset) {
	switch (cset.t) {
		case cff_CHARSET_ISOADOBE:
		case cff_CHARSET_EXPERT:
		case cff_CHARSET_EXPERTSUBSET: {
			return bufnew();
		}
		case cff_CHARSET_FORMAT0: {
			caryll_Buffer *blob = bufnew();
			blob->size = 1 + cset.s * 2;
			NEW(blob->data, blob->size);
			blob->data[0] = 0;
			for (uint32_t i = 0; i < cset.s; i++)
				blob->data[1 + 2 * i] = cset.f0.glyph[i] / 256, blob->data[2 + 2 * i] = cset.f0.glyph[i] % 256;
			blob->cursor = blob->size;
			return blob;
		}
		case cff_CHARSET_FORMAT1: {
			caryll_Buffer *blob = bufnew();
			blob->size = 1 + cset.s * 3;
			NEW(blob->data, blob->size);
			blob->data[0] = 1;
			for (uint32_t i = 0; i < cset.s; i++)
				blob->data[1 + 3 * i] = cset.f1.range1[i].first / 256,
				                   blob->data[2 + 3 * i] = cset.f1.range1[i].first % 256,
				                   blob->data[3 + 3 * i] = cset.f1.range1[i].nleft;
			return blob;
		}
		case cff_CHARSET_FORMAT2: {
			caryll_Buffer *blob = bufnew();
			blob->size = 1 + cset.s * 4;
			NEW(blob->data, blob->size);
			blob->data[0] = 2;
			for (uint32_t i = 0; i < cset.s; i++)
				blob->data[1 + 4 * i] = cset.f2.range2[i].first / 256,
				                   blob->data[2 + 4 * i] = cset.f2.range2[i].first % 256,
				                   blob->data[3 + 4 * i] = cset.f2.range2[i].nleft / 256,
				                   blob->data[4 + 4 * i] = cset.f2.range2[i].nleft % 256;
			blob->cursor = blob->size;
			return blob;
		}
	}
	return NULL;
}

void cff_close_Charset(cff_Charset cset) {
	switch (cset.t) {
		case cff_CHARSET_EXPERT:
		case cff_CHARSET_EXPERTSUBSET:
		case cff_CHARSET_ISOADOBE:
			break;
		case cff_CHARSET_FORMAT0:
			if (cset.f0.glyph != NULL) FREE(cset.f0.glyph);
			break;
		case cff_CHARSET_FORMAT1:
			if (cset.f1.range1 != NULL) FREE(cset.f1.range1);
			break;
		case cff_CHARSET_FORMAT2:
			if (cset.f2.range2 != NULL) FREE(cset.f2.range2);
			break;
	}
}
