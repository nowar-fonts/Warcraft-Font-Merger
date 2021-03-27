#ifndef CARYLL_OTF_WRITER_STAT_H
#define CARYLL_OTF_WRITER_STAT_H

#include "otfcc/font.h"
#include "table/all.h"

void otfcc_statFont(otfcc_Font *font, const otfcc_Options *options);
void otfcc_unstatFont(otfcc_Font *font, const otfcc_Options *options);
#endif
