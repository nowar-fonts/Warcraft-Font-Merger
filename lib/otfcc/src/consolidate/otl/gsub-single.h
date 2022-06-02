#ifndef CARYLL_FONTOPS_OTL_GSUB_SINGLE_H
#define CARYLL_FONTOPS_OTL_GSUB_SINGLE_H
#include "common.h"

bool consolidate_gsub_single(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                             const otfcc_Options *options);

#endif
