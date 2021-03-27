#ifndef CARYLL_TABLE_CPAL_H
#define CARYLL_TABLE_CPAL_H

#include "otfcc/table/CPAL.h"

table_CPAL *otfcc_readCPAL(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpCPAL(const table_CPAL *table, json_value *root, const otfcc_Options *options);
table_CPAL *otfcc_parseCPAL(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildCPAL(const table_CPAL *cpal, const otfcc_Options *options);

#endif
