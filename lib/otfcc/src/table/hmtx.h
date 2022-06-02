#ifndef CARYLL_TABLE_HMTX_H
#define CARYLL_TABLE_HMTX_H

#include "otfcc/table/hmtx.h"

table_hmtx *otfcc_readHmtx(const otfcc_Packet packet, const otfcc_Options *options, table_hhea *hhea, table_maxp *maxp);
caryll_Buffer *otfcc_buildHmtx(const table_hmtx *table, glyphid_t count_a, glyphid_t count_k,
                               const otfcc_Options *options);

#endif
