#ifndef CARYLL_TABLE_TSI_H
#define CARYLL_TABLE_TSI_H

#include "otfcc/table/_TSI.h"

table_TSI *otfcc_readTSI(const otfcc_Packet packet, const otfcc_Options *options, uint32_t tagIndex, uint32_t tagText);
void otfcc_dumpTSI(const table_TSI *table, json_value *root, const otfcc_Options *options, const char *tag);
table_TSI *otfcc_parseTSI(const json_value *root, const otfcc_Options *options, const char *tag);

// TSI tables has two parts: an Index part and a Text part, so we need a struct.
typedef struct {
	caryll_Buffer *indexPart;
	caryll_Buffer *textPart;
} tsi_BuildTarget;

tsi_BuildTarget otfcc_buildTSI(const table_TSI *TSI, const otfcc_Options *options);

#endif
