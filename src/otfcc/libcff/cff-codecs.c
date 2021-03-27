/*
  Codec of CFF file format and Type2 CharString.
*/

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "libcff.h"

/*
  Number in Type2
  1 32 -246      [-107, 107]           b0 - 139
  2 247-250      [108, 1131]           (b0 - 247) * 256 + b1 + 108
  2 251-254      [-1132, -108]         -(b0 - 251) * 256 - b1 - 108
  3 28 (int16_t) [-32768, 32767]       b1 << 8 | b2
  5 29 (int32_t) [-(2^31), (2^32 - 1)] b1 << 24 | b2 << 16 | b3 << 8 | b4
  * 30 (double)
*/

caryll_Buffer *cff_encodeCffOperator(int32_t val) {
	if (val > 256) {
		return bufninit(2, val / 256, val % 256);
	} else {
		return bufninit(1, val);
	}
}

caryll_Buffer *cff_encodeCffInteger(int32_t val) {
	if (val >= -107 && val <= 107) {
		return bufninit(1, val + 139);
	} else if (val >= 108 && val <= 1131) {
		val -= 108;
		return bufninit(2, (val >> 8) + 247, val & 0xff);
	} else if (val >= -1131 && val <= -108) {
		val = -108 - val;
		return bufninit(2, (val >> 8) + 251, val & 0xff);
	} else if (val >= -32768 && val < 32768) {
		return bufninit(3, 28, val >> 8, val & 0xff);
	} else {
		/* In dict data we have 4 byte ints, in type2 strings we don't */
		return bufninit(5, 29, (val >> 24) & 0xff, (val >> 16) & 0xff, (val >> 8) & 0xff, val & 0xff);
	}
}

// -2.25       -> 1e e2 a2 5f
// 0.140541E-3 -> 1e 0a 14 05 41 c3 ff
caryll_Buffer *cff_encodeCffFloat(double val) {
	caryll_Buffer *blob = bufnew();
	uint32_t i, j = 0;
	uint8_t temp[32] = {0};

	if (val == 0.0) {
		blob->size = 2;
		NEW(blob->data, blob->size);
		blob->data[0] = 30;
		blob->data[1] = 0x0f;
	} else {
		uint32_t niblen = 0;
		uint8_t *array;
		sprintf((char *)temp, "%.13g", val);

		for (i = 0; i < strlen((char *)temp);) {
			if (temp[i] == '.')
				niblen++, i++;
			else if (temp[i] >= '0' && temp[i] <= '9')
				niblen++, i++;
			else if (temp[i] == 'e' && temp[i + 1] == '-')
				niblen++, i += 2;
			else if (temp[i] == 'e' && temp[i + 1] == '+')
				niblen++, i += 2;
			else if (temp[i] == '-')
				niblen++, i++;
		}

		blob->size = 2 + niblen / 2;
		NEW(blob->data, blob->size);
		blob->data[0] = 30;

		if (niblen % 2 != 0) {
			NEW(array, (niblen + 1));
			array[niblen] = 0x0f;
		} else {
			NEW(array, (niblen + 2));
			array[niblen + 1] = 0x0f;
			array[niblen] = 0x0f;
		}

		for (i = 0; i < strlen((char *)temp);) {
			if (temp[i] == '.')
				array[j++] = 0x0a, i++;
			else if (temp[i] >= '0' && temp[i] <= '9')
				array[j++] = temp[i] - '0', i++;
			else if (temp[i] == 'e' && temp[i + 1] == '-')
				array[j++] = 0x0c, i += 2;
			else if (temp[i] == 'e' && temp[i + 1] == '+')
				array[j++] = 0x0b, i += 2;
			else if (temp[i] == '-')
				array[j++] = 0x0e, i++;
		}

		for (i = 1; i < blob->size; i++) {
			blob->data[i] = array[(i - 1) * 2] * 16 + array[(i - 1) * 2 + 1];
		}

		FREE(array);
	}

	return blob;
}

uint32_t cff_decodeCS2Token(const uint8_t *start, cff_Value *val) {
	uint32_t advance = 0;

	if (*start <= 27) {
		val->t = CS2_OPERATOR;

		if (*start <= 11) {
			val->i = *start;
			advance = 1;
		} else if (*start == 12) {
			val->i = *start << 8 | *(start + 1);
			advance = 2;
		} else if (*start >= 13 && *start <= 18) {
			val->i = *start;
			advance = 1;
		} else if (*start >= 19 && *start <= 20) {
			val->i = *start;
			advance = 1;
		} else if (*start >= 21 && *start <= 27) {
			val->i = *start;
			advance = 1;
		}
	} else if (*start == 28) {
		val->t = CS2_OPERAND;
		val->i = (int16_t)(*(start + 1) << 8 | *(start + 2));
		advance = 3;
	} else if (*start >= 29 && *start <= 31) {
		val->t = CS2_OPERATOR;
		val->i = *start;
		advance = 1;
	} else if (*start >= 32 && *start <= 254) {
		val->t = CS2_OPERAND;
		if (*start >= 32 && *start <= 246) {
			val->i = (int32_t)(*start - 139);
			advance = 1;
		} else if (*start >= 247 && *start <= 250) {
			val->i = (int32_t)((*start - 247) * 256 + *(start + 1) + 108);
			advance = 2;
		} else if (*start >= 251 && *start <= 254) {
			val->i = (int32_t)(-((*start - 251) * 256) - *(start + 1) - 108);
			advance = 2;
		}
	} else if (*start == 255) {
		val->t = CS2_FRACTION;
		int16_t integerPart = start[1] << 8 | start[2];
		uint16_t fractionPart = start[3] << 8 | start[4];
		val->d = (double)(integerPart + fractionPart / 65536.0);
		advance = 5;
	}

	if (val->t == CS2_OPERAND) val->d = (double)val->i, val->t = CS2_FRACTION;

	return advance;
}

// decode integer
static uint32_t cff_dec_i(const uint8_t *start, cff_Value *val) {
	uint8_t b0 = *start, b1, b2, b3, b4;
	uint32_t len = 0;

	if (b0 >= 32 && b0 <= 246) {
		val->i = b0 - 139;
		len = 1;
	} else if (b0 >= 247 && b0 <= 250) {
		b1 = *(start + 1);
		val->i = (b0 - 247) * 256 + b1 + 108;
		len = 2;
	} else if (b0 >= 251 && b0 <= 254) {
		b1 = *(start + 1);
		val->i = -(b0 - 251) * 256 - b1 - 108;
		len = 2;
	} else if (b0 == 28) {
		b1 = *(start + 1);
		b2 = *(start + 2);
		val->i = (b1 << 8) | b2;
		len = 3;
	} else if (b0 == 29) {
		b1 = *(start + 1);
		b2 = *(start + 2);
		b3 = *(start + 3);
		b4 = *(start + 4);
		val->i = b1 << 24 | b2 << 16 | b3 << 8 | b4;
		len = 5;
	}

	val->t = cff_INTEGER;
	return len;
}

static const int nibble_attr[15] = {1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 0, 1};
static const char *nibble_symb[15] = {"0", "1", "2", "3", "4", "5", "6", "7", "8", "9", ".", "E", "E-", "", "-"};

// decode double
static uint32_t cff_dec_r(const uint8_t *start, cff_Value *val) {
	uint8_t restr[72] = {0};
	size_t str_len = 0;
	uint32_t len;
	uint8_t a, b;

	const uint8_t *nibst = start + 1;

	while (1) {
		a = *nibst / 16;
		b = *nibst % 16;

		if (a != 15)
			str_len += nibble_attr[a];
		else
			break;

		if (b != 15)
			str_len += nibble_attr[b];
		else
			break;

		nibst++;
	}

	len = (uint32_t)(nibst - start + 1);
	nibst = start + 1;

	while (1) {
		a = *nibst / 16;
		b = *nibst % 16;

		if (a != 0x0f)
			strcat((char *)restr, nibble_symb[a]);
		else
			break;

		if (b != 0x0f)
			strcat((char *)restr, nibble_symb[b]);
		else
			break;

		nibst++;
	}

	val->d = atof((char *)restr);
	val->t = cff_DOUBLE;

	return len;
}

// decode operator
static uint32_t cff_dec_o(const uint8_t *start, cff_Value *val) {
	uint8_t b0 = *start, b1;
	uint32_t len = 0;

	if (b0 <= 21) {
		if (b0 != 12) {
			val->i = b0;
			len = 1;
		} else {
			b1 = *(start + 1);
			val->i = b0 * 256 + b1;
			len = 2;
		}
	}

	val->t = cff_OPERATOR;

	return len;
}

// error in parsing, return a integer
static uint32_t cff_dec_e(const uint8_t *start, cff_Value *val) {
	printf("Undefined Byte in CFF: %d.\n", *start);
	val->i = *start;
	val->t = cff_INTEGER;
	return 1;
}

static uint32_t (*_de_t2[256])(const uint8_t *, cff_Value *) = {
    cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o,
    cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o, cff_dec_o,
    cff_dec_o, cff_dec_o, cff_dec_e, cff_dec_e, cff_dec_e, cff_dec_e, cff_dec_e, cff_dec_e, cff_dec_i, cff_dec_i,
    cff_dec_r, cff_dec_e, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i,
    cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_i, cff_dec_e};

uint32_t cff_decodeCffToken(const uint8_t *start, cff_Value *val) {
	return _de_t2[*start](start, val);
}
