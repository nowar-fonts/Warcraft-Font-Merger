#include "unicodeconv.h"

// Brought from libXML2.
sds utf16le_to_utf8(const uint8_t *inb, int inlenb) {
	uint16_t *in = (uint16_t *)inb;
	uint16_t *inend;
	uint32_t c, d, inlen;
	int bits;

	if ((inlenb % 2) == 1) (inlenb)--;
	inlen = inlenb / 2;
	inend = in + inlen;
	// pass 1: calculate bytes used for output
	uint32_t bytesNeeded = 0;
	while (in < inend) {
		c = *in++;
		if ((c & 0xFC00) == 0xD800) { // surrogates
			if (in >= inend) {        // (in > inend) shouldn't happens
				break;
			}
			d = *in++;
			if ((d & 0xFC00) == 0xDC00) {
				c &= 0x03FF;
				c <<= 10;
				c |= d & 0x03FF;
				c += 0x10000;
			}
		}
		if (c < 0x80) {
			bytesNeeded += 1;
		} else if (c < 0x800) {
			bytesNeeded += 2;
		} else if (c < 0x10000) {
			bytesNeeded += 3;
		} else {
			bytesNeeded += 4;
		}
	}
	in = (uint16_t *)inb;
	sds out = sdsnewlen(NULL, bytesNeeded);
	sds out0 = out;

	while (in < inend) {
		c = *in++;
		if ((c & 0xFC00) == 0xD800) {
			if (in >= inend) { break; }
			d = *in++;
			if ((d & 0xFC00) == 0xDC00) {
				c &= 0x03FF;
				c <<= 10;
				c |= d & 0x03FF;
				c += 0x10000;
			}
		}

		if (c < 0x80) {
			*out++ = c;
			bits = -6;
		} else if (c < 0x800) {
			*out++ = ((c >> 6) & 0x1F) | 0xC0;
			bits = 0;
		} else if (c < 0x10000) {
			*out++ = ((c >> 12) & 0x0F) | 0xE0;
			bits = 6;
		} else {
			*out++ = ((c >> 18) & 0x07) | 0xF0;
			bits = 12;
		}

		for (; bits >= 0; bits -= 6) {
			*out++ = ((c >> bits) & 0x3F) | 0x80;
		}
	}
	return out0;
}

sds utf16be_to_utf8(const uint8_t *inb, int inlenb) {
	uint16_t *in = (uint16_t *)inb;
	uint16_t *inend;
	uint32_t c, d, inlen;
	uint8_t *tmp;
	int bits;

	if ((inlenb % 2) == 1) (inlenb)--;
	inlen = inlenb / 2;
	inend = in + inlen;
	uint32_t bytesNeeded = 0;
	while (in < inend) {
		{
			tmp = (uint8_t *)in;
			c = *tmp++;
			c = c << 8;
			c = c | (uint32_t)*tmp;
			in++;
		}
		if ((c & 0xFC00) == 0xD800) {
			if (in >= inend) { break; }
			{
				tmp = (uint8_t *)in;
				d = *tmp++;
				d = d << 8;
				d = d | (uint32_t)*tmp;
				in++;
			}
			if ((d & 0xFC00) == 0xDC00) {
				c &= 0x03FF;
				c <<= 10;
				c |= d & 0x03FF;
				c += 0x10000;
			}
		}
		if (c < 0x80) {
			bytesNeeded += 1;
		} else if (c < 0x800) {
			bytesNeeded += 2;
		} else if (c < 0x10000) {
			bytesNeeded += 3;
		} else {
			bytesNeeded += 4;
		}
	}
	in = (uint16_t *)inb;
	sds out = sdsnewlen(NULL, bytesNeeded);
	sds out0 = out;

	while (in < inend) {
		{
			tmp = (uint8_t *)in;
			c = *tmp++;
			c = c << 8;
			c = c | (uint32_t)*tmp;
			in++;
		}
		if ((c & 0xFC00) == 0xD800) {
			if (in >= inend) { break; }
			{
				tmp = (uint8_t *)in;
				d = *tmp++;
				d = d << 8;
				d = d | (uint32_t)*tmp;
				in++;
			}
			if ((d & 0xFC00) == 0xDC00) {
				c &= 0x03FF;
				c <<= 10;
				c |= d & 0x03FF;
				c += 0x10000;
			}
		}

		if (c < 0x80) {
			*out++ = c;
			bits = -6;
		} else if (c < 0x800) {
			*out++ = ((c >> 6) & 0x1F) | 0xC0;
			bits = 0;
		} else if (c < 0x10000) {
			*out++ = ((c >> 12) & 0x0F) | 0xE0;
			bits = 6;
		} else {
			*out++ = ((c >> 18) & 0x07) | 0xF0;
			bits = 12;
		}

		for (; bits >= 0; bits -= 6) {
			*out++ = ((c >> bits) & 0x3F) | 0x80;
		}
	}
	return out0;
}

uint8_t *utf8toutf16be(sds _in, size_t *out_bytes) {
	if (!_in) {
		*out_bytes = 0;
		return NULL;
	}
	sds in = _in;
	size_t inlen = sdslen(in);
	char *inend = in + inlen;

	uint32_t wordsNeeded = 0;
	uint8_t trailing = 0;
	uint32_t c = 0;
	while (in < inend) {
		uint8_t d = *in++;
		if (d < 0x80) {
			c = d;
			trailing = 0;
		} else if (d < 0xC0) {
			break; // trailing byte in leading position
		} else if (d < 0xE0) {
			c = d & 0x1F;
			trailing = 1;
		} else if (d < 0xF0) {
			c = d & 0x0F;
			trailing = 2;
		} else if (d < 0xF8) {
			c = d & 0x07;
			trailing = 3;
		} else {
			break;
		}
		if (inend - in < trailing) { break; }
		for (; trailing; trailing--) {
			if ((in >= inend) || (((d = *in++) & 0xC0) != 0x80)) break;
			c <<= 6;
			c |= d & 0x3F;
		}
		if (c < 0x10000) {
			wordsNeeded += 1;
		} else if (c < 0x110000) {
			wordsNeeded += 2;
		}
	}
	uint8_t *_out = malloc(2 * wordsNeeded * sizeof(uint8_t));
	uint8_t *out = _out;
	in = _in;
	while (in < inend) {
		uint8_t d = *in++;
		if (d < 0x80) {
			c = d;
			trailing = 0;
		} else if (d < 0xC0) {
			break; // trailing byte in leading position
		} else if (d < 0xE0) {
			c = d & 0x1F;
			trailing = 1;
		} else if (d < 0xF0) {
			c = d & 0x0F;
			trailing = 2;
		} else if (d < 0xF8) {
			c = d & 0x07;
			trailing = 3;
		} else {
			break;
		}
		if (inend - in < trailing) { break; }
		for (; trailing; trailing--) {
			if ((in >= inend) || (((d = *in++) & 0xC0) != 0x80)) break;
			c <<= 6;
			c |= d & 0x3F;
		}
		if (c < 0x10000) {
			*(out++) = (c >> 8) & 0xFF;
			*(out++) = c & 0xFF;
		} else if (c < 0x110000) {
			uint16_t tmp1 = 0xD800 | (c >> 10);
			*(out++) = (tmp1 >> 8) & 0xFF;
			*(out++) = tmp1 & 0xFF;
			uint16_t tmp2 = 0xDC00 | (c & 0x03FF);
			*(out++) = (tmp2 >> 8) & 0xFF;
			*(out++) = tmp2 & 0xFF;
		}
	}
	*out_bytes = wordsNeeded * 2;
	return _out;
}
