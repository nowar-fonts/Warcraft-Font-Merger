#ifndef CARYLL_TABLE_CMAP_H
#define CARYLL_TABLE_CMAP_H

#include "otfcc/table/cmap.h"

table_cmap *otfcc_readCmap(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpCmap(const table_cmap *cmap, json_value *root, const otfcc_Options *options);
table_cmap *otfcc_parseCmap(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildCmap(const table_cmap *cmap, const otfcc_Options *options);

#endif
