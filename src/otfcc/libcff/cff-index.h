#ifndef CARYLL_cff_INDEX_H
#define CARYLL_cff_INDEX_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "support/util.h"

#include "cff-util.h"
#include "cff-value.h"

typedef enum { CFF_INDEX_16, CFF_INDEX_32 } cff_IndexCountType;

typedef struct {
	cff_IndexCountType countType;
	arity_t count;
	uint8_t offSize;
	uint32_t *offset;
	uint8_t *data;
} cff_Index;

extern caryll_ElementInterfaceOf(cff_Index) {
	caryll_RT(cff_Index);
	void (*empty)(cff_Index * i);
	uint32_t (*getLength)(const cff_Index *i);
	void (*parse)(uint8_t * data, uint32_t pos, cff_Index * in);
	cff_Index *(*fromCallback)(void *context, uint32_t length, caryll_Buffer *(*fn)(void *, uint32_t));
	caryll_Buffer *(*build)(const cff_Index *index);
}
cff_iIndex;

#endif
