#include "base64.h"
#include "../otfcc-alloc.h"

static const uint8_t base64_table[64] =
    "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

uint8_t *base64_encode(const uint8_t *src, size_t len, size_t *out_len) {
	uint8_t *out, *pos;
	const uint8_t *end, *in;
	size_t olen;

	olen = (len + 3 - 1) / 3 * 4; /* 3-byte blocks to 4-byte */
	olen++;                       /* nul termination */
	out = __caryll_malloc(sizeof(uint8_t) * olen);
	if (out == NULL) return NULL;

	end = src + len;
	in = src;
	pos = out;
	while (end - in >= 3) {
		*pos++ = base64_table[in[0] >> 2];
		*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
		*pos++ = base64_table[((in[1] & 0x0f) << 2) | (in[2] >> 6)];
		*pos++ = base64_table[in[2] & 0x3f];
		in += 3;
	}

	if (end - in) {
		*pos++ = base64_table[in[0] >> 2];
		if (end - in == 1) {
			*pos++ = base64_table[(in[0] & 0x03) << 4];
			*pos++ = '=';
		} else {
			*pos++ = base64_table[((in[0] & 0x03) << 4) | (in[1] >> 4)];
			*pos++ = base64_table[(in[1] & 0x0f) << 2];
		}
		*pos++ = '=';
	}

	*pos = '\0';
	if (out_len) *out_len = pos - out;
	return out;
}

uint8_t *base64_decode(const uint8_t *src, size_t len, size_t *out_len) {
	uint8_t dtable[256], *out, *pos, in[4], block[4], tmp;
	size_t i, count;

	memset(dtable, 0x80, 256);
	for (i = 0; i < sizeof(base64_table); i++)
		dtable[base64_table[i]] = i;
	dtable['='] = 0;

	count = 0;
	for (i = 0; i < len; i++) {
		if (dtable[src[i]] != 0x80) count++;
	}

	if (count % 4) return NULL;

	pos = out = __caryll_malloc(sizeof(uint8_t) * count);
	if (out == NULL) return NULL;

	count = 0;
	for (i = 0; i < len; i++) {
		tmp = dtable[src[i]];
		if (tmp == 0x80) continue;

		in[count] = src[i];
		block[count] = tmp;
		count++;
		if (count == 4) {
			*pos++ = (block[0] << 2) | (block[1] >> 4);
			*pos++ = (block[1] << 4) | (block[2] >> 2);
			*pos++ = (block[2] << 6) | block[3];
			count = 0;
		}
	}

	if (pos > out) {
		if (in[2] == '=')
			pos -= 2;
		else if (in[3] == '=')
			pos--;
	}

	*out_len = pos - out;
	return out;
}
