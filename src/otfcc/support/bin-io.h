#ifndef CARYLL_SUPPORT_BIN_IO_H
#define CARYLL_SUPPORT_BIN_IO_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#ifndef INLINE
#ifdef _MSC_VER
#define INLINE __forceinline /* use __forceinline (VC++ specific) */
#else
#define INLINE inline /* use standard inline */
#endif
#endif

static INLINE bool otfcc_check_endian(void) {
	union {
		uint8_t i1[2];
		uint16_t i2;
	} check_union = {.i2 = 1}; // if you don't have a new VC, upgrade it

	return (check_union.i1[0] == 1);
}

static INLINE uint16_t otfcc_endian_convert16(uint16_t i) {
	if (otfcc_check_endian()) {
		union {
			uint8_t i1[2];
			uint16_t i2;
		} src, des;

		src.i2 = i;

		des.i1[0] = src.i1[1];
		des.i1[1] = src.i1[0];

		return des.i2;
	} else {
		return i;
	}
}

static INLINE uint32_t otfcc_endian_convert32(uint32_t i) {
	if (otfcc_check_endian()) {
		union {
			uint8_t i1[4];
			uint32_t i4;
		} src, des;

		src.i4 = i;

		des.i1[0] = src.i1[3];
		des.i1[1] = src.i1[2];
		des.i1[2] = src.i1[1];
		des.i1[3] = src.i1[0];

		return des.i4;
	} else {
		return i;
	}
}

static INLINE uint64_t otfcc_endian_convert64(uint64_t i) {
	if (otfcc_check_endian()) {
		union {
			uint8_t i1[8];
			uint64_t i8;
		} src, des;

		src.i8 = i;

		des.i1[0] = src.i1[7];
		des.i1[1] = src.i1[6];
		des.i1[2] = src.i1[5];
		des.i1[3] = src.i1[4];
		des.i1[4] = src.i1[3];
		des.i1[5] = src.i1[2];
		des.i1[6] = src.i1[1];
		des.i1[7] = src.i1[0];

		return des.i8;
	} else {
		return i;
	}
}

static INLINE uint16_t otfcc_get16u(FILE *file) {
	uint16_t tmp;
	size_t sizeRead = fread(&tmp, 2, 1, file);
	if (!sizeRead) {
		fprintf(stderr, "File corruption of terminated unexpectedly.\n");
		exit(EXIT_FAILURE);
	}
	return otfcc_endian_convert16(tmp);
}

static INLINE uint32_t otfcc_get32u(FILE *file) {
	uint32_t tmp;
	size_t sizeRead = fread(&tmp, 4, 1, file);
	if (!sizeRead) {
		fprintf(stderr, "File corruption of terminated unexpectedly.\n");
		exit(EXIT_FAILURE);
	}
	return otfcc_endian_convert32(tmp);
}

static INLINE uint64_t otfcc_get64u(FILE *file) {
	uint64_t tmp;
	size_t sizeRead = fread(&tmp, 8, 1, file);
	if (!sizeRead) {
		fprintf(stderr, "File corruption of terminated unexpectedly.\n");
		exit(EXIT_FAILURE);
	}
	return otfcc_endian_convert64(tmp);
}

// data reader
static INLINE uint8_t read_8u(const uint8_t *src) {
	return src[0];
}
static INLINE uint16_t read_16u(const uint8_t *src) {
	uint16_t b0 = ((uint16_t)src[0]) << 8;
	uint16_t b1 = ((uint16_t)src[1]);
	return (b0 | b1);
}
static INLINE uint32_t read_24u(const uint8_t *src) {
	uint32_t b0 = ((uint32_t)src[0]) << 16;
	uint32_t b1 = ((uint32_t)src[1]) << 8;
	uint32_t b2 = ((uint32_t)src[2]);
	return (b0 | b1 | b2);
}
static INLINE uint32_t read_32u(const uint8_t *src) {
	uint32_t b0 = ((uint32_t)src[0]) << 24;
	uint32_t b1 = ((uint32_t)src[1]) << 16;
	uint32_t b2 = ((uint32_t)src[2]) << 8;
	uint32_t b3 = ((uint32_t)src[3]);
	return (b0 | b1 | b2 | b3);
}
static INLINE uint64_t read_64u(const uint8_t *src) {
	uint64_t b0 = ((uint64_t)src[0]) << 56;
	uint64_t b1 = ((uint64_t)src[1]) << 48;
	uint64_t b2 = ((uint64_t)src[2]) << 40;
	uint64_t b3 = ((uint64_t)src[3]) << 32;
	uint64_t b4 = ((uint64_t)src[4]) << 24;
	uint64_t b5 = ((uint64_t)src[5]) << 16;
	uint64_t b6 = ((uint64_t)src[6]) << 8;
	uint64_t b7 = ((uint64_t)src[7]);
	return (b0 | b1 | b2 | b3 | b4 | b5 | b6 | b7);
}
static INLINE int8_t read_8s(const uint8_t *src) {
	return (int8_t)read_8u(src);
}
static INLINE int16_t read_16s(const uint8_t *src) {
	return (int16_t)read_16u(src);
}
static INLINE int32_t read_32s(const uint8_t *src) {
	return (int32_t)read_32u(src);
}
static INLINE int64_t read_64s(const uint8_t *src) {
	return (int64_t)read_64u(src);
}

static INLINE uint16_t be16(uint16_t x) {
	return ((x & 0xff) << 8) | ((x & 0xFF00) >> 8);
}
static INLINE uint32_t be32(uint32_t x) {
	return ((x & 0x000000ff) << 24) | ((x & 0x0000FF00) << 8) | ((x & 0x00FF0000) >> 8) |
	       ((x & 0xFF000000) >> 24);
}

#endif
