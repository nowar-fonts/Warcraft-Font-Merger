#ifndef CARYLL_TABLE_CVT_H
#define CARYLL_TABLE_CVT_H

#include "otfcc/table/cvt.h"

table_cvt *otfcc_readCvt(const otfcc_Packet packet, const otfcc_Options *options, uint32_t tag);
void otfcc_dumpCvt(const table_cvt *table, json_value *root, const otfcc_Options *options, const char *tag);
table_cvt *otfcc_parseCvt(const json_value *root, const otfcc_Options *options, const char *tag);
caryll_Buffer *otfcc_buildCvt(const table_cvt *table, const otfcc_Options *options);

#endif
