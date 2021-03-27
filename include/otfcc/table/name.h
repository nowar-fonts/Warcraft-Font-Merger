#ifndef CARYLL_INCLUDE_TABLE_NAME_H
#define CARYLL_INCLUDE_TABLE_NAME_H

#include "table-common.h"

typedef struct {
	uint16_t platformID;
	uint16_t encodingID;
	uint16_t languageID;
	uint16_t nameID;
	sds nameString;
} otfcc_NameRecord;
extern caryll_ElementInterface(otfcc_NameRecord) otfcc_iNameRecord;
typedef caryll_Vector(otfcc_NameRecord) table_name;
extern caryll_VectorInterface(table_name, otfcc_NameRecord) table_iName;

#endif
