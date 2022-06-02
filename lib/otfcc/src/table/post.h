#ifndef CARYLL_TABLE_POST_H
#define CARYLL_TABLE_POST_H

#include "otfcc/table/post.h"

table_post *otfcc_readPost(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpPost(const table_post *table, json_value *root, const otfcc_Options *options);
table_post *otfcc_parsePost(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildPost(const table_post *post, otfcc_GlyphOrder *glyphorder, const otfcc_Options *options);

#endif
