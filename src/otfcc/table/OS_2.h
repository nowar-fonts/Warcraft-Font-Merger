#ifndef CARYLL_TABLE_OS_2_H
#define CARYLL_TABLE_OS_2_H

#include "otfcc/table/OS_2.h"

table_OS_2 *otfcc_readOS_2(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpOS_2(const table_OS_2 *table, json_value *root, const otfcc_Options *options);
table_OS_2 *otfcc_parseOS_2(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildOS_2(const table_OS_2 *os_2, const otfcc_Options *options);

#endif
