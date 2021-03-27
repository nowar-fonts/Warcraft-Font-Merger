#ifndef CARYLL_BK_BLOCK_H
#define CARYLL_BK_BLOCK_H

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>
#include <limits.h>
#include "caryll/ownership.h"
#include "support/otfcc-alloc.h"
#include "caryll/buffer.h"

struct __caryll_bkblock;
typedef enum {
	bkover = 0,    // nothing
	b8 = 1,        // byte
	b16 = 2,       // short
	b32 = 3,       // long
	p16 = 0x10,    // 16-bit offset, p = pointer to block
	p32 = 0x11,    // 32-bit offset, p = pointer to block
	sp16 = 0x80,   // 16-bit offset, p = pointer to block, marked as compact
	sp32 = 0x81,   // 32-bit offset, p = pointer to block, marked as compact
	bkcopy = 0xFE, // Embed another block
	bkembed = 0xFF // Embed another block
} bk_CellType;
typedef enum { VISIT_WHITE, VISIT_GRAY, VISIT_BLACK } bk_cell_visit_state;

typedef struct {
	bk_CellType t;
	union {
		uint32_t z;
		struct __caryll_bkblock *p;
	};
} bk_Cell;

typedef struct __caryll_bkblock {
	bk_cell_visit_state _visitstate;
	uint32_t _index;
	uint32_t _height;
	uint32_t _depth;
	uint32_t length;
	uint32_t free;
	bk_Cell *cells;
} bk_Block;

bk_Block *_bkblock_init();
bk_Block *bk_new_Block(int type0, ...);
bk_Block *bk_push(bk_Block *b, int type0, ...);
bk_Block *bk_newBlockFromStringLen(size_t len, const char *str);
bk_Block *bk_newBlockFromBuffer(MOVE caryll_Buffer *buf);
bk_Block *bk_newBlockFromBufferCopy(OBSERVE caryll_Buffer *buf);
bool bk_cellIsPointer(bk_Cell *cell);
void bk_printBlock(bk_Block *b);

#endif
