#ifndef CARYLL_TABLE_META_H
#define CARYLL_TABLE_META_H

#include "otfcc/table/meta.h"

table_meta *otfcc_readMeta(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpMeta(const table_meta *table, json_value *root, const otfcc_Options *options);
table_meta *otfcc_parseMeta(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildMeta(const table_meta *meta, const otfcc_Options *options);

#endif
