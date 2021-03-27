#ifndef CARYLL_TABLE_MAXP_H
#define CARYLL_TABLE_MAXP_H

#include "otfcc/table/maxp.h"

table_maxp *otfcc_readMaxp(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpMaxp(const table_maxp *table, json_value *root, const otfcc_Options *options);
table_maxp *otfcc_parseMaxp(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildMaxp(const table_maxp *maxp, const otfcc_Options *options);

#endif
