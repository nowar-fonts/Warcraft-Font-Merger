#ifndef CARYLL_FONTOPS_OTL_GSUB_MULTI_H
#define CARYLL_FONTOPS_OTL_GSUB_MULTI_H
#include "common.h"

bool consolidate_gsub_multi(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                            const otfcc_Options *options);
bool consolidate_gsub_alternative(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                                  const otfcc_Options *options);

#endif
