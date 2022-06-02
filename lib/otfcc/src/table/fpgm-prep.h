#ifndef CARYLL_TABLE_FPGM_PREP_H
#define CARYLL_TABLE_FPGM_PREP_H

#include "otfcc/table/fpgm-prep.h"

table_fpgm_prep *otfcc_readFpgmPrep(const otfcc_Packet packet, const otfcc_Options *options, uint32_t tag);
void table_dumpTableFpgmPrep(const table_fpgm_prep *table, json_value *root, const otfcc_Options *options,
                             const char *tag);
table_fpgm_prep *otfcc_parseFpgmPrep(const json_value *root, const otfcc_Options *options, const char *tag);
caryll_Buffer *otfcc_buildFpgmPrep(const table_fpgm_prep *table, const otfcc_Options *options);

#endif
