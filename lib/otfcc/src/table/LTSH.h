#ifndef CARYLL_TABLE_LTSH_H
#define CARYLL_TABLE_LTSH_H

#include "otfcc/table/LTSH.h"

table_LTSH *otfcc_readLTSH(const otfcc_Packet packet, const otfcc_Options *options);
caryll_Buffer *otfcc_buildLTSH(const table_LTSH *ltsh, const otfcc_Options *options);

#endif
