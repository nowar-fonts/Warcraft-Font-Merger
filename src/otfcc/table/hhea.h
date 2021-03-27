#ifndef CARYLL_TABLE_HHEA_H
#define CARYLL_TABLE_HHEA_H

#include "otfcc/table/hhea.h"

table_hhea *otfcc_newHhea();
table_hhea *otfcc_readHhea(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpHhea(const table_hhea *table, json_value *root, const otfcc_Options *options);
table_hhea *otfcc_parseHhea(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildHhea(const table_hhea *hhea, const otfcc_Options *options);

#endif
