#ifndef CARYLL_TABLE_GASP_H
#define CARYLL_TABLE_GASP_H

#include "otfcc/table/gasp.h"

table_gasp *otfcc_readGasp(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpGasp(const table_gasp *table, json_value *root, const otfcc_Options *options);
table_gasp *otfcc_parseGasp(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildGasp(const table_gasp *table, const otfcc_Options *options);

#endif
