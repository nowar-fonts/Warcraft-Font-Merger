#ifndef CARYLL_TABLE_BASE_H
#define CARYLL_TABLE_BASE_H

#include "otfcc/table/BASE.h"

table_BASE *otfcc_readBASE(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpBASE(const table_BASE *base, json_value *root, const otfcc_Options *options);
table_BASE *otfcc_parseBASE(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildBASE(const table_BASE *base, const otfcc_Options *options);

#endif
