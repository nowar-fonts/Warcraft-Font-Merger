#ifndef CARYLL_TABLE_GDEF_H
#define CARYLL_TABLE_GDEF_H

#include "otfcc/table/GDEF.h"

table_GDEF *otfcc_readGDEF(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpGDEF(const table_GDEF *gdef, json_value *root, const otfcc_Options *options);
table_GDEF *otfcc_parseGDEF(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildGDEF(const table_GDEF *gdef, const otfcc_Options *options);

#endif
