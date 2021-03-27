#ifndef CARYLL_TABLE_SVG_H
#define CARYLL_TABLE_SVG_H

#include "otfcc/table/SVG.h"

table_SVG *otfcc_readSVG(const otfcc_Packet packet, const otfcc_Options *options);
void otfcc_dumpSVG(const table_SVG *svg, json_value *root, const otfcc_Options *options);
table_SVG *otfcc_parseSVG(const json_value *root, const otfcc_Options *options);
caryll_Buffer *otfcc_buildSVG(const table_SVG *svg, const otfcc_Options *options);

#endif
