#ifndef CARYLL_TABLE_HEAD_H
#define CARYLL_TABLE_HEAD_H

#include "otfcc/table/head.h"

table_head *otfcc_readHead(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpHead(const table_head *table, json_value *root, const otfcc_Options *options);
table_head *otfcc_parseHead(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildHead(const table_head *head, const otfcc_Options *options);

#endif
