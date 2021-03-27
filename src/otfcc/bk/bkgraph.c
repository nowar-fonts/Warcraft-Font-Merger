#include "bkgraph.h"

static bk_GraphNode *_bkgraph_grow(bk_Graph *f) {
	if (f->free) {
		f->length++;
		f->free--;
	} else {
		f->length = f->length + 1;
		f->free = (f->length >> 1) & 0xFFFFFF;
		RESIZE(f->entries, f->length + f->free);
	}
	return &(f->entries[f->length - 1]);
}

static uint32_t dfs_insert_cells(bk_Block *b, bk_Graph *f, uint32_t *order) {
	if (!b || b->_visitstate == VISIT_GRAY) return 0;
	if (b->_visitstate == VISIT_BLACK) return b->_height;
	b->_visitstate = VISIT_GRAY;
	uint32_t height = 0;
	for (uint32_t j = 0; j < b->length; j++) {
		bk_Cell *cell = &(b->cells[j]);
		if (bk_cellIsPointer(cell) && cell->p) {
			uint32_t thatHeight = dfs_insert_cells(cell->p, f, order);
			if (thatHeight + 1 > height) height = thatHeight + 1;
		}
	}
	bk_GraphNode *e = _bkgraph_grow(f);
	e->alias = 0;
	e->block = b;
	*order += 1;
	e->order = *order;
	e->height = b->_height = height;
	b->_visitstate = VISIT_BLACK;
	return height;
}

static int _by_height(const void *_a, const void *_b) {
	const bk_GraphNode *a = _a;
	const bk_GraphNode *b = _b;
	return a->height == b->height ? a->order - b->order : b->height - a->height;
}

static int _by_order(const void *_a, const void *_b) {
	const bk_GraphNode *a = _a;
	const bk_GraphNode *b = _b;
	return a->block && b->block && a->block->_visitstate != b->block->_visitstate // Visited first
	           ? b->block->_visitstate - a->block->_visitstate
	           : a->block && b->block && a->block->_depth != b->block->_depth // By depth
	                 ? a->block->_depth - b->block->_depth
	                 : b->order - a->order; // By order
}

bk_Graph *bk_newGraphFromRootBlock(bk_Block *b) {
	bk_Graph *forest;
	NEW(forest);
	uint32_t tsOrder = 0;
	dfs_insert_cells(b, forest, &tsOrder);
	qsort(forest->entries, forest->length, sizeof(bk_GraphNode), _by_height);
	for (uint32_t j = 0; j < forest->length; j++) {
		forest->entries[j].block->_index = j;
		forest->entries[j].alias = j;
	}
	return forest;
}

void bk_delete_Graph(bk_Graph *f) {
	if (!f || !f->entries) return;
	for (uint32_t j = 0; j < f->length; j++) {
		bk_Block *b = f->entries[j].block;
		if (b && b->cells) FREE(b->cells);
		FREE(b);
	}
	FREE(f->entries);
	FREE(f);
}

static uint32_t gethash(bk_Block *b) {
	uint32_t h = 5381;
	for (uint32_t j = 0; j < b->length; j++) {
		h = ((h << 5) + h) + b->cells[j].t;
		h = ((h << 5) + h);
		switch (b->cells[j].t) {
			case b8:
			case b16:
			case b32:
				h += b->cells[j].z;
				break;
			case p16:
			case p32:
			case sp16:
			case sp32:
				if (b->cells[j].p) { h += b->cells[j].p->_index; }
				break;
			default:
				break;
		}
	}
	return h;
}

static bool compareblock(bk_Block *a, bk_Block *b) {
	if (!a && !b) return true;
	if (!a || !b) return false;
	if (a->length != b->length) return false;
	for (uint32_t j = 0; j < a->length; j++) {
		if (a->cells[j].t != b->cells[j].t) return false;
		switch (a->cells[j].t) {
			case b8:
			case b16:
			case b32:
				if (a->cells[j].z != b->cells[j].z) return false;
				break;
			case p16:
			case p32:
			case sp16:
			case sp32:
				if (a->cells[j].p != b->cells[j].p) return false;
				break;
			default:
				break;
		}
	}
	return true;
}
static bool compareEntry(bk_GraphNode *a, bk_GraphNode *b) {
	if (a->hash != b->hash) return false;
	return compareblock(a->block, b->block);
}

static void replaceptr(bk_Graph *f, bk_Block *b) {
	for (uint32_t j = 0; j < b->length; j++) {
		switch (b->cells[j].t) {
			case p16:
			case p32:
			case sp16:
			case sp32:
				if (b->cells[j].p) {
					uint32_t index = b->cells[j].p->_index;
					while (f->entries[index].alias != index) {
						index = f->entries[index].alias;
					}
					b->cells[j].p = f->entries[index].block;
				}
				break;
			default:
				break;
		}
	}
}

void bk_minimizeGraph(bk_Graph *f) {
	uint32_t rear = (uint32_t)(f->length - 1);
	while (rear > 0) {
		uint32_t front = rear;
		while (f->entries[front].height == f->entries[rear].height && front > 0) {
			front--;
		}
		front++;
		for (uint32_t j = front; j <= rear; j++) {
			f->entries[j].hash = gethash(f->entries[j].block);
		}
		for (uint32_t j = front; j <= rear; j++) {
			bk_GraphNode *a = &(f->entries[j]);
			if (a->alias == j) {
				for (uint32_t k = j + 1; k <= rear; k++) {
					bk_GraphNode *b = &(f->entries[k]);
					if (b->alias == k && compareEntry(a, b)) { b->alias = j; }
				}
			}
		}
		// replace pointers with aliased
		for (uint32_t j = 0; j < front; j++) {
			replaceptr(f, f->entries[j].block);
		}
		rear = front - 1;
	}
}

static size_t otfcc_bkblock_size(bk_Block *b) {
	size_t size = 0;
	for (uint32_t j = 0; j < b->length; j++)
		switch (b->cells[j].t) {
			case b8:
				size += 1;
				break;
			case b16:
			case p16:
			case sp16:
				size += 2;
				break;
			case b32:
			case p32:
			case sp32:
				size += 4;
				break;
			default:
				break;
		}
	return size;
}

static uint32_t getoffset(size_t *offsets, bk_Block *ref, bk_Block *target, uint8_t bits) {
	size_t offref = offsets[ref->_index];
	size_t offtgt = offsets[target->_index];
	/*
	if (offtgt < offref || (offtgt - offref) >> bits) {
	    fprintf(stderr, "[otfcc-fea] Warning : Unable to fit offset %d into %d bits.\n",
	(int32_t)(offtgt - offref), bits);
	}
	*/
	return (uint32_t)(offtgt - offref);
}
static int64_t getoffset_untangle(size_t *offsets, bk_Block *ref, bk_Block *target) {
	size_t offref = offsets[ref->_index];
	size_t offtgt = offsets[target->_index];
	return (int64_t)(offtgt - offref);
}
static void escalate_sppointers(bk_Block *b, bk_Graph *f, uint32_t *order, uint32_t depth) {
	if (!b) return;
	for (uint32_t j = 0; j < b->length; j++) {
		bk_Cell *cell = &(b->cells[j]);
		if (bk_cellIsPointer(cell) && cell->p && cell->t >= sp16) {
			escalate_sppointers(cell->p, f, order, depth);
		}
	}
	b->_depth = depth;
	*order += 1;
	f->entries[b->_index].order = *order;
}
static void dfs_attract_cells(bk_Block *b, bk_Graph *f, uint32_t *order, uint32_t depth) {
	if (!b) return;
	if (b->_visitstate != VISIT_WHITE) {
		if (b->_depth < depth) { b->_depth = depth; }
		return;
	}
	b->_visitstate = VISIT_GRAY;
	for (uint32_t j = b->length; j-- > 0;) {
		bk_Cell *cell = &(b->cells[j]);
		if (bk_cellIsPointer(cell) && cell->p) { dfs_attract_cells(cell->p, f, order, depth + 1); }
	}
	*order += 1;
	f->entries[b->_index].order = *order;
	escalate_sppointers(b, f, order, depth);
	b->_visitstate = VISIT_BLACK;
}

static void attract_bkgraph(bk_Graph *f) {
	// Clear the visit state of all blocks
	for (uint32_t j = 0; j < f->length; j++) {
		f->entries[j].block->_visitstate = VISIT_WHITE;
		f->entries[j].order = 0;
		f->entries[j].block->_index = j;
		f->entries[j].block->_depth = 0;
	}
	uint32_t order = 0;
	dfs_attract_cells(f->entries[0].block, f, &order, 0);
	qsort(f->entries, f->length, sizeof(bk_GraphNode), _by_order);
	for (uint32_t j = 0; j < f->length; j++) {
		f->entries[j].block->_index = j;
	}
}

static bool try_untabgle_block(bk_Graph *f, bk_Block *b, size_t *offsets, uint16_t passes) {
	bool didCopy = false;
	for (uint32_t j = 0; j < b->length; j++) {
		switch (b->cells[j].t) {
			case p16:
			case sp16:
				if (b->cells[j].p) {
					int64_t offset = getoffset_untangle(offsets, b, b->cells[j].p);
					if (offset < 0 || offset > 0xFFFF) {
						bk_GraphNode *e = _bkgraph_grow(f);
						e->order = 0;
						e->alias = 0;
						e->block = bk_new_Block(bkcopy, b->cells[j].p, bkover);
						b->cells[j].t = sp16;
						b->cells[j].p = e->block;
						didCopy = true;
					}
				}
				break;
			default:
				break;
		}
	}
	return didCopy;
}

static bool try_untangle(bk_Graph *f, uint16_t passes) {
	size_t *offsets;
	NEW(offsets, f->length + 1);
	offsets[0] = 0;
	for (uint32_t j = 0; j < f->length; j++) {
		if (f->entries[j].block->_visitstate == VISIT_BLACK) {
			offsets[j + 1] = offsets[j] + otfcc_bkblock_size(f->entries[j].block);
		} else {
			offsets[j + 1] = offsets[j];
		}
	}
	uint32_t totalBlocks = f->length;
	bool didUntangle = false;
	for (uint32_t j = 0; j < totalBlocks; j++) {
		if (f->entries[j].block->_visitstate == VISIT_BLACK) {
			bool didCopy = try_untabgle_block(f, f->entries[j].block, offsets, passes);
			didUntangle = didUntangle || didCopy;
		}
	}
	FREE(offsets);
	return didUntangle;
}

static void otfcc_build_bkblock(caryll_Buffer *buf, bk_Block *b, size_t *offsets) {
	for (uint32_t j = 0; j < b->length; j++) {
		switch (b->cells[j].t) {
			case b8:
				bufwrite8(buf, b->cells[j].z);
				break;
			case b16:
				bufwrite16b(buf, b->cells[j].z);
				break;
			case b32:
				bufwrite32b(buf, b->cells[j].z);
				break;
			case p16:
			case sp16:
				if (b->cells[j].p) {
					bufwrite16b(buf, getoffset(offsets, b, b->cells[j].p, 16));
				} else {
					bufwrite16b(buf, 0);
				}
				break;
			case p32:
			case sp32:
				if (b->cells[j].p) {
					bufwrite32b(buf, getoffset(offsets, b, b->cells[j].p, 32));
				} else {
					bufwrite32b(buf, 0);
				}
				break;
			default:
				break;
		}
	}
}

caryll_Buffer *bk_build_Graph(bk_Graph *f) {
	caryll_Buffer *buf = bufnew();
	size_t *offsets;
	NEW(offsets, f->length + 1);

	offsets[0] = 0;
	for (uint32_t j = 0; j < f->length; j++) {
		if (f->entries[j].block->_visitstate == VISIT_BLACK) {
			offsets[j + 1] = offsets[j] + otfcc_bkblock_size(f->entries[j].block);
		} else {
			offsets[j + 1] = offsets[j];
		}
	}
	for (uint32_t j = 0; j < f->length; j++) {
		if (f->entries[j].block->_visitstate == VISIT_BLACK) {
			otfcc_build_bkblock(buf, f->entries[j].block, offsets);
		}
	}
	FREE(offsets);
	return buf;
}

size_t bk_estimateSizeOfGraph(bk_Graph *f) {
	size_t *offsets;
	NEW(offsets, f->length + 1);

	offsets[0] = 0;
	for (uint32_t j = 0; j < f->length; j++) {
		if (f->entries[j].block->_visitstate == VISIT_BLACK) {
			offsets[j + 1] = offsets[j] + otfcc_bkblock_size(f->entries[j].block);
		} else {
			offsets[j + 1] = offsets[j];
		}
	}
	size_t estimatedSize = offsets[f->length];
	FREE(offsets);
	return estimatedSize;
}

void bk_untangleGraph(/*BORROW*/ bk_Graph *f) {
	uint16_t passes = 0;
	bool tangled = false;
	attract_bkgraph(f);
	do {
		tangled = try_untangle(f, passes);
		if (tangled) { attract_bkgraph(f); }
		passes++;
	} while (tangled && passes < 16);
}

caryll_Buffer *bk_build_Block(/*MOVE*/ bk_Block *root) {
	bk_Graph *f = bk_newGraphFromRootBlock(root);
	bk_minimizeGraph(f);
	bk_untangleGraph(f);
	caryll_Buffer *buf = bk_build_Graph(f);
	bk_delete_Graph(f);
	return buf;
}
caryll_Buffer *bk_build_Block_noMinimize(/*MOVE*/ bk_Block *root) {
	bk_Graph *f = bk_newGraphFromRootBlock(root);
	bk_untangleGraph(f);
	caryll_Buffer *buf = bk_build_Graph(f);
	bk_delete_Graph(f);
	return buf;
}
