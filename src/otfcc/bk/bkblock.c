#include "bkblock.h"

static void bkblock_acells(bk_Block *b, uint32_t len) {
	if (len <= b->length + b->free) {
		// We have enough space
		b->free -= len - b->length;
		b->length = len;
	} else {
		// allocate space
		b->length = len;
		b->free = (len >> 1) & 0xFFFFFF;
		RESIZE(b->cells, b->length + b->free);
	}
}
bool bk_cellIsPointer(bk_Cell *cell) {
	return cell->t >= p16;
}

static bk_Cell *bkblock_grow(bk_Block *b, uint32_t len) {
	uint32_t olen = b->length;
	bkblock_acells(b, olen + len);
	return &(b->cells[olen]);
}

bk_Block *_bkblock_init() {
	bk_Block *b;
	NEW(b);
	bkblock_acells(b, 0);
	return b;
}

void bkblock_pushint(bk_Block *b, bk_CellType type, uint32_t x) {
	bk_Cell *cell = bkblock_grow(b, 1);
	cell->t = type;
	cell->z = x;
}
void bkblock_pushptr(bk_Block *b, bk_CellType type, bk_Block *p) {
	bk_Cell *cell = bkblock_grow(b, 1);
	cell->t = type;
	cell->p = p;
}

static void vbkpushitems(bk_Block *b, bk_CellType type0, va_list ap) {
	bk_CellType curtype = type0;
	while (curtype) {
		if (curtype == bkcopy || curtype == bkembed) {
			bk_Block *par = va_arg(ap, bk_Block *);
			if (par && par->cells) {
				for (uint32_t j = 0; j < par->length; j++) {
					if (bk_cellIsPointer(par->cells + j)) {
						bkblock_pushptr(b, par->cells[j].t, par->cells[j].p);
					} else {
						bkblock_pushint(b, par->cells[j].t, par->cells[j].z);
					}
				}
			}
			if (curtype == bkembed && par) {
				FREE(par->cells);
				FREE(par);
			}
		} else if (curtype < p16) {
			uint32_t par = va_arg(ap, int);
			bkblock_pushint(b, curtype, par);
		} else {
			bk_Block *par = va_arg(ap, bk_Block *);
			bkblock_pushptr(b, curtype, par);
		}
		curtype = va_arg(ap, int);
	}
}

bk_Block *bk_new_Block(int type0, ...) {
	va_list ap;
	va_start(ap, type0);
	bk_Block *b = _bkblock_init();
	vbkpushitems(b, type0, ap);
	va_end(ap);
	return b;
}

bk_Block *bk_push(bk_Block *b, int type0, ...) {
	va_list ap;
	va_start(ap, type0);
	vbkpushitems(b, type0, ap);
	va_end(ap);
	return b;
}

bk_Block *bk_newBlockFromStringLen(size_t len, const char *str) {
	if (!str) return NULL;
	bk_Block *b = bk_new_Block(bkover);
	for (size_t j = 0; j < len; j++) {
		bkblock_pushint(b, b8, str[j]);
	}
	return b;
}

bk_Block *bk_newBlockFromBuffer(MOVE caryll_Buffer *buf) {
	if (!buf) return NULL;
	bk_Block *b = bk_new_Block(bkover);
	for (size_t j = 0; j < buf->size; j++) {
		bkblock_pushint(b, b8, buf->data[j]);
	}
	buffree(buf);
	return b;
}
bk_Block *bk_newBlockFromBufferCopy(OBSERVE caryll_Buffer *buf) {
	if (!buf) return NULL;
	bk_Block *b = bk_new_Block(bkover);
	for (size_t j = 0; j < buf->size; j++) {
		bkblock_pushint(b, b8, buf->data[j]);
	}
	return b;
}

void bk_printBlock(bk_Block *b) {
	fprintf(stderr, "Block size %08x\n", (uint32_t)b->length);
	fprintf(stderr, "------------------\n");
	for (uint32_t j = 0; j < b->length; j++) {
		if (bk_cellIsPointer(b->cells + j)) {
			if (b->cells[j].p) {
				fprintf(stderr, "  %3d %p[%d]\n", b->cells[j].t, b->cells[j].p,
				        b->cells[j].p->_index);
			} else {
				fprintf(stderr, "  %3d [NULL]\n", b->cells[j].t);
			}
		} else {
			fprintf(stderr, "  %3d %d\n", b->cells[j].t, b->cells[j].z);
		}
	}
	fprintf(stderr, "------------------\n");
}
