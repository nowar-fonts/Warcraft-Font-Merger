#include "caryll/buffer.h"
#include "support/otfcc-alloc.h"

caryll_Buffer *bufnew() {
	caryll_Buffer *buf;
	NEW(buf);
	buf->size = buf->free = 0;
	return buf;
}
void buffree(caryll_Buffer *buf) {
	if (!buf) return;
	if (buf->data) FREE(buf->data);
	FREE(buf);
}
size_t buflen(caryll_Buffer *buf) {
	return buf->size;
}
size_t bufpos(caryll_Buffer *buf) {
	return buf->cursor;
}
void bufseek(caryll_Buffer *buf, size_t pos) {
	buf->cursor = pos;
}
void bufclear(caryll_Buffer *buf) {
	buf->cursor = 0;
	buf->free = buf->size + buf->free;
	buf->size = 0;
}

static void bufbeforewrite(caryll_Buffer *buf, size_t towrite) {
	size_t currentSize = buf->size;
	size_t allocated = buf->size + buf->free;
	size_t required = buf->cursor + towrite;
	if (required < currentSize) {
		// Completely overlap.
		return;
	} else if (required <= allocated) {
		// Within range without reallocation
		buf->size = required;
		buf->free = allocated - buf->size;
	} else {
		// Needs realloc
		buf->size = required;
		buf->free = required; // Double growth
		if (buf->free > 0x1000000) { buf->free = 0x1000000; }
		RESIZE(buf->data, buf->size + buf->free);
	}
}
void bufwrite8(caryll_Buffer *buf, uint8_t byte) {
	bufbeforewrite(buf, 1);
	buf->data[buf->cursor++] = byte;
}
void bufwrite16l(caryll_Buffer *buf, uint16_t x) {
	bufbeforewrite(buf, 2);
	buf->data[buf->cursor++] = x & 0xFF;
	buf->data[buf->cursor++] = (x >> 8) & 0xFF;
}
void bufwrite16b(caryll_Buffer *buf, uint16_t x) {
	bufbeforewrite(buf, 2);
	buf->data[buf->cursor++] = (x >> 8) & 0xFF;
	buf->data[buf->cursor++] = x & 0xFF;
}
void bufwrite24l(caryll_Buffer *buf, uint32_t x) {
	bufbeforewrite(buf, 3);
	buf->data[buf->cursor++] = x & 0xFF;
	buf->data[buf->cursor++] = (x >> 8) & 0xFF;
	buf->data[buf->cursor++] = (x >> 16) & 0xFF;
}
void bufwrite24b(caryll_Buffer *buf, uint32_t x) {
	bufbeforewrite(buf, 3);
	buf->data[buf->cursor++] = (x >> 16) & 0xFF;
	buf->data[buf->cursor++] = (x >> 8) & 0xFF;
	buf->data[buf->cursor++] = x & 0xFF;
}
void bufwrite32l(caryll_Buffer *buf, uint32_t x) {
	bufbeforewrite(buf, 4);
	buf->data[buf->cursor++] = x & 0xFF;
	buf->data[buf->cursor++] = (x >> 8) & 0xFF;
	buf->data[buf->cursor++] = (x >> 16) & 0xFF;
	buf->data[buf->cursor++] = (x >> 24) & 0xFF;
}
void bufwrite32b(caryll_Buffer *buf, uint32_t x) {
	bufbeforewrite(buf, 4);
	buf->data[buf->cursor++] = (x >> 24) & 0xFF;
	buf->data[buf->cursor++] = (x >> 16) & 0xFF;
	buf->data[buf->cursor++] = (x >> 8) & 0xFF;
	buf->data[buf->cursor++] = x & 0xFF;
}
void bufwrite64l(caryll_Buffer *buf, uint64_t x) {
	bufbeforewrite(buf, 8);
	buf->data[buf->cursor++] = x & 0xFF;
	buf->data[buf->cursor++] = (x >> 8) & 0xFF;
	buf->data[buf->cursor++] = (x >> 16) & 0xFF;
	buf->data[buf->cursor++] = (x >> 24) & 0xFF;
	buf->data[buf->cursor++] = (x >> 32) & 0xFF;
	buf->data[buf->cursor++] = (x >> 40) & 0xFF;
	buf->data[buf->cursor++] = (x >> 48) & 0xFF;
	buf->data[buf->cursor++] = (x >> 56) & 0xFF;
}
void bufwrite64b(caryll_Buffer *buf, uint64_t x) {
	bufbeforewrite(buf, 8);
	buf->data[buf->cursor++] = (x >> 56) & 0xFF;
	buf->data[buf->cursor++] = (x >> 48) & 0xFF;
	buf->data[buf->cursor++] = (x >> 40) & 0xFF;
	buf->data[buf->cursor++] = (x >> 32) & 0xFF;
	buf->data[buf->cursor++] = (x >> 24) & 0xFF;
	buf->data[buf->cursor++] = (x >> 16) & 0xFF;
	buf->data[buf->cursor++] = (x >> 8) & 0xFF;
	buf->data[buf->cursor++] = x & 0xFF;
}

caryll_Buffer *bufninit(uint32_t n, ...) {
	caryll_Buffer *buf = bufnew();
	bufbeforewrite(buf, n);
	va_list ap;
	va_start(ap, n);
	for (uint16_t j = 0; j < n; j++) {
		bufwrite8(buf, (uint8_t)va_arg(ap, int));
	}
	va_end(ap);
	return buf;
}
void bufnwrite8(caryll_Buffer *buf, uint32_t n, ...) {
	bufbeforewrite(buf, n);
	va_list ap;
	va_start(ap, n);
	for (uint16_t j = 0; j < n; j++) {
		bufwrite8(buf, (uint8_t)va_arg(ap, int));
	}
	va_end(ap);
}

void bufwrite_sds(caryll_Buffer *buf, sds str) {
	if (!str) return;
	size_t len = sdslen(str);
	if (!len) return;
	bufbeforewrite(buf, len);
	memcpy(buf->data + buf->cursor, str, len);
	buf->cursor += len;
}
void bufwrite_str(caryll_Buffer *buf, const char *str) {
	if (!str) return;
	size_t len = strlen(str);
	if (!len) return;
	bufbeforewrite(buf, len);
	memcpy(buf->data + buf->cursor, str, len);
	buf->cursor += len;
}
void bufwrite_bytes(caryll_Buffer *buf, size_t len, const uint8_t *str) {
	if (!str) return;
	if (!len) return;
	bufbeforewrite(buf, len);
	memcpy(buf->data + buf->cursor, str, len);
	buf->cursor += len;
}
void bufwrite_buf(caryll_Buffer *buf, caryll_Buffer *that) {
	if (!that || !that->data) return;
	size_t len = buflen(that);
	bufbeforewrite(buf, len);
	memcpy(buf->data + buf->cursor, that->data, len);
	buf->cursor += len;
}
void bufwrite_bufdel(caryll_Buffer *buf, caryll_Buffer *that) {
	if (!that) return;
	if (!that->data) {
		buffree(that);
		return;
	}
	size_t len = buflen(that);
	bufbeforewrite(buf, len);
	memcpy(buf->data + buf->cursor, that->data, len);
	buffree(that);
	buf->cursor += len;
}

void buflongalign(caryll_Buffer *buf) {
	size_t cp = buf->cursor;
	bufseek(buf, buflen(buf));
	if (buflen(buf) % 4 == 1) {
		bufwrite8(buf, 0);
		bufwrite8(buf, 0);
		bufwrite8(buf, 0);
	} else if (buflen(buf) % 4 == 2) {
		bufwrite8(buf, 0);
		bufwrite8(buf, 0);
	} else if (buflen(buf) % 4 == 3) {
		bufwrite8(buf, 0);
	}
	bufseek(buf, cp);
}

// bufpingpong16b writes a buffer and an offset towards it.
// [ ^                            ] + ###### that
//   ^cp             ^offset
//                           |
//                           V
// [ @^              ######       ] , and the value of [@] equals to the former
// offset.
//    ^cp                  ^offset
// Common in writing OpenType features.
void bufping16b(caryll_Buffer *buf, size_t *offset, size_t *cp) {
	bufwrite16b(buf, *offset);
	*cp = buf->cursor;
	bufseek(buf, *offset);
}
void bufping16bd(caryll_Buffer *buf, size_t *offset, size_t *shift, size_t *cp) {
	bufwrite16b(buf, *offset - *shift);
	*cp = buf->cursor;
	bufseek(buf, *offset);
}
void bufpong(caryll_Buffer *buf, size_t *offset, size_t *cp) {
	*offset = buf->cursor;
	bufseek(buf, *cp);
}
void bufpingpong16b(caryll_Buffer *buf, caryll_Buffer *that, size_t *offset, size_t *cp) {
	bufwrite16b(buf, *offset);
	*cp = buf->cursor;
	bufseek(buf, *offset);
	bufwrite_bufdel(buf, that);
	*offset = buf->cursor;
	bufseek(buf, *cp);
}

void bufprint(caryll_Buffer *buf) {
	for (size_t j = 0; j < buf->size; j++) {
		if (j % 16) fprintf(stderr, " ");
		fprintf(stderr, "%02X", buf->data[j]);
		if (j % 16 == 15) fprintf(stderr, "\n");
	}
	fprintf(stderr, "\n");
}
