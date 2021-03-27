#ifndef CARYLL_INCLUDE_BUFFER_H
#define CARYLL_INCLUDE_BUFFER_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdarg.h>
#include "dep/sds.h"

typedef struct caryll_Buffer {
	size_t cursor;
	size_t size;
	size_t free;
	uint8_t *data;
} caryll_Buffer;

caryll_Buffer *bufnew();
caryll_Buffer *bufninit(uint32_t n, ...);
void buffree(caryll_Buffer *buf);
size_t buflen(caryll_Buffer *buf);
size_t bufpos(caryll_Buffer *buf);
void bufseek(caryll_Buffer *buf, size_t pos);
void bufclear(caryll_Buffer *buf);

void bufwrite8(caryll_Buffer *buf, uint8_t byte);
void bufwrite16l(caryll_Buffer *buf, uint16_t x);
void bufwrite16b(caryll_Buffer *buf, uint16_t x);
void bufwrite24l(caryll_Buffer *buf, uint32_t x);
void bufwrite24b(caryll_Buffer *buf, uint32_t x);
void bufwrite32l(caryll_Buffer *buf, uint32_t x);
void bufwrite32b(caryll_Buffer *buf, uint32_t x);
void bufwrite64l(caryll_Buffer *buf, uint64_t x);
void bufwrite64b(caryll_Buffer *buf, uint64_t x);

void bufnwrite8(caryll_Buffer *buf, uint32_t n, ...);

void bufwrite_sds(caryll_Buffer *buf, sds str);
void bufwrite_str(caryll_Buffer *buf, const char *str);
void bufwrite_bytes(caryll_Buffer *buf, size_t size, const uint8_t *str);
void bufwrite_buf(caryll_Buffer *buf, caryll_Buffer *that);
void bufwrite_bufdel(caryll_Buffer *buf, caryll_Buffer *that);

void bufping16b(caryll_Buffer *buf, size_t *offset, size_t *cp);
void bufping16bd(caryll_Buffer *buf, size_t *offset, size_t *shift, size_t *cp);
void bufpingpong16b(caryll_Buffer *buf, caryll_Buffer *that, size_t *offset, size_t *cp);
void bufpong(caryll_Buffer *buf, size_t *offset, size_t *cp);

void bufprint(caryll_Buffer *buf);

void buflongalign(caryll_Buffer *buf);

#endif
