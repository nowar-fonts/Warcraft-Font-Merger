#pragma once

#include <stdint.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdlib.h>

#include <otfcc/buffer.hpp>

#include "bkblock.h"

typedef struct {
	uint32_t alias;
	uint32_t order;
	uint32_t height;
	uint32_t hash;
	bk_Block *block;
} bk_GraphNode;

typedef struct {
	uint32_t length;
	uint32_t free;
	bk_GraphNode *entries;
} bk_Graph;

bk_Graph *bk_newGraphFromRootBlock(bk_Block *b);
void bk_delete_Graph(/*MOVE*/ bk_Graph *f);
void bk_minimizeGraph(/*BORROW*/ bk_Graph *f);
void bk_untangleGraph(/*BORROW*/ bk_Graph *f);
otfcc::buffer bk_build_Graph(/*BORROW*/ bk_Graph *f);
otfcc::buffer bk_build_Block(/*MOVE*/ bk_Block *root);
otfcc::buffer bk_build_Block_noMinimize(/*MOVE*/ bk_Block *root);
size_t bk_estimateSizeOfGraph(bk_Graph *f);
