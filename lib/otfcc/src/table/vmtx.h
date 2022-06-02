#ifndef CARYLL_TABLE_VMTX_H
#define CARYLL_TABLE_VMTX_H

#include "otfcc/table/vmtx.h"

table_vmtx *otfcc_readVmtx(const otfcc_Packet packet, const otfcc_Options *options, table_vhea *vhea, table_maxp *maxp);
caryll_Buffer *otfcc_buildVmtx(const table_vmtx *table, glyphid_t count_a, glyphid_t count_k,
                               const otfcc_Options *options);

#endif
