#ifndef CARYLL_TABLE_COLR_H
#define CARYLL_TABLE_COLR_H

#include "otfcc/table/COLR.h"

table_COLR *otfcc_readCOLR(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpCOLR(const table_COLR *table, json_value *root, const otfcc_Options *options);
table_COLR *otfcc_parseCOLR(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildCOLR(const table_COLR *colr, const otfcc_Options *options);

#endif
