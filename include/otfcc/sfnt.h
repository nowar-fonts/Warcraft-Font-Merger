#ifndef CARYLL_INCLUDE_OTFCC_SFNT_H
#define CARYLL_INCLUDE_OTFCC_SFNT_H

#include <stdint.h>
#include <stdio.h>

typedef struct {
	uint32_t tag;
	uint32_t checkSum;
	uint32_t offset;
	uint32_t length;
	uint8_t *data;
} otfcc_PacketPiece;

typedef struct {
	uint32_t sfnt_version;
	uint16_t numTables;
	uint16_t searchRange;
	uint16_t entrySelector;
	uint16_t rangeShift;
	otfcc_PacketPiece *pieces;
} otfcc_Packet;

typedef struct {
	uint32_t type;
	uint32_t count;
	uint32_t *offsets;
	otfcc_Packet *packets;
} otfcc_SplineFontContainer;

otfcc_SplineFontContainer *otfcc_readSFNT(FILE *file);
void otfcc_deleteSFNT(otfcc_SplineFontContainer *font);

#endif
