#include "support/util.h"
#include "otfcc/sfnt-builder.h"

static uint32_t buf_checksum(caryll_Buffer *buffer) {
	uint32_t actualLength = (uint32_t)buflen(buffer);
	buflongalign(buffer);
	uint32_t sum = 0;
	{
		uint32_t *start = (uint32_t *)buffer->data;
		uint32_t *end = start + ((actualLength + 3) & ~3) / sizeof(uint32_t);
		while (start < end) {
			sum += otfcc_endian_convert32(*start++);
		}
	}
	return sum;
}

static otfcc_SFNTTableEntry *createSegment(uint32_t tag, caryll_Buffer *buffer) {
	otfcc_SFNTTableEntry *table;
	NEW(table);
	table->tag = tag;
	table->length = (uint32_t)buflen(buffer);
	buflongalign(buffer);
	table->buffer = buffer;

	uint32_t sum = 0;
	{
		uint32_t *start = (uint32_t *)buffer->data;
		uint32_t *end = start + ((table->length + 3) & ~3) / sizeof(uint32_t);
		while (start < end) {
			sum += otfcc_endian_convert32(*start++);
		}
	}
	table->checksum = sum;
	return table;
}

otfcc_SFNTBuilder *otfcc_newSFNTBuilder(uint32_t header, const otfcc_Options *options) {
	otfcc_SFNTBuilder *builder;
	NEW(builder);
	builder->count = 0;
	builder->header = header;
	builder->tables = NULL;
	builder->options = options;
	return builder;
}

void otfcc_deleteSFNTBuilder(otfcc_SFNTBuilder *builder) {
	if (!builder) return;
	otfcc_SFNTTableEntry *item, *tmp;
	HASH_ITER(hh, builder->tables, item, tmp) {
		HASH_DEL(builder->tables, item);
		buffree(item->buffer);
		FREE(item);
	}
	FREE(builder);
}

void otfcc_SFNTBuilder_pushTable(otfcc_SFNTBuilder *builder, uint32_t tag, caryll_Buffer *buffer) {
	if (!builder || !buffer) return;
	otfcc_SFNTTableEntry *item;
	const otfcc_Options *options = builder->options;
	HASH_FIND_INT(builder->tables, &tag, item);
	if (!item) {
		item = createSegment(tag, buffer);
		HASH_ADD_INT(builder->tables, tag, item);
		logProgress("OpenType table %c%c%c%c successfully built.\n", (tag >> 24) & 0xff,
		            (tag >> 16) & 0xff, (tag >> 8) & 0xff, tag & 0xff);
	} else {
		buffree(buffer);
	}
}

static int byTag(otfcc_SFNTTableEntry *a, otfcc_SFNTTableEntry *b) {
	return (a->tag - b->tag);
}

caryll_Buffer *otfcc_SFNTBuilder_serialize(otfcc_SFNTBuilder *builder) {
	caryll_Buffer *buffer = bufnew();
	if (!builder) return buffer;
	uint16_t nTables = HASH_COUNT(builder->tables);
	uint16_t searchRange = (nTables < 16 ? 8 : nTables < 32 ? 16 : nTables < 64 ? 32 : 64) * 16;
	bufwrite32b(buffer, builder->header);
	bufwrite16b(buffer, nTables);
	bufwrite16b(buffer, searchRange);
	bufwrite16b(buffer, (nTables < 16 ? 3 : nTables < 32 ? 4 : nTables < 64 ? 5 : 6));
	bufwrite16b(buffer, nTables * 16 - searchRange);

	otfcc_SFNTTableEntry *table;
	size_t offset = 12 + nTables * 16;
	size_t headOffset = offset;
	HASH_SORT(builder->tables, byTag);
	foreach_hash(table, builder->tables) {
		// write table directory
		bufwrite32b(buffer, table->tag);
		bufwrite32b(buffer, table->checksum);
		bufwrite32b(buffer, (uint32_t)offset);
		bufwrite32b(buffer, table->length);
		size_t cp = buffer->cursor;
		bufseek(buffer, offset);
		bufwrite_buf(buffer, table->buffer);
		bufseek(buffer, cp);
		// record where the [head] is
		if (table->tag == 'head') { headOffset = offset; }
		offset += buflen(table->buffer);
	}

	// write head.checksumAdjust
	uint32_t wholeChecksum = buf_checksum(buffer);
	bufseek(buffer, headOffset + 8);
	bufwrite32b(buffer, 0xB1B0AFBA - wholeChecksum);
	return buffer;
}
