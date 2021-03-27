#ifndef CARYLL_FONTOPS_OTL_GSUB_REVERSE_H
#define CARYLL_FONTOPS_OTL_GSUB_REVERSE_H
#include "common.h"

bool consolidate_gsub_reverse(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                              const otfcc_Options *options);

#endif
