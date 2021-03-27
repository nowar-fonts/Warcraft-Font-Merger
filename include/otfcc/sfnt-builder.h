#ifndef CARYLL_SFNT_BUILDER_H
#define CARYLL_SFNT_BUILDER_H

#include "primitives.h"
#include "caryll/buffer.h"
#include "otfcc/options.h"
#include "dep/uthash.h"

typedef struct {
	int tag;
	uint32_t length;
	uint32_t checksum;
	caryll_Buffer *buffer;
	UT_hash_handle hh;
} otfcc_SFNTTableEntry;

typedef struct {
	uint32_t count;
	uint32_t header;
	otfcc_SFNTTableEntry *tables;
	const otfcc_Options *options;
} otfcc_SFNTBuilder;

otfcc_SFNTBuilder *otfcc_newSFNTBuilder(uint32_t header, const otfcc_Options *options);
void otfcc_SFNTBuilder_pushTable(otfcc_SFNTBuilder *builder, uint32_t tag, caryll_Buffer *buffer);
void otfcc_deleteSFNTBuilder(otfcc_SFNTBuilder *builder);

caryll_Buffer *otfcc_SFNTBuilder_serialize(otfcc_SFNTBuilder *builder);

#endif
