#ifndef CARYLL_FONTOPS_OTL_MARK_H
#define CARYLL_FONTOPS_OTL_MARK_H
#include "common.h"

bool consolidate_mark_to_single(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                                const otfcc_Options *options);
bool consolidate_mark_to_ligature(otfcc_Font *font, table_OTL *table, otl_Subtable *_subtable,
                                  const otfcc_Options *options);

#endif
