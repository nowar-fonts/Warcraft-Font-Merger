#ifndef CARYLL_cff_DICT_H
#define CARYLL_cff_DICT_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "support/util.h"
#include "cff-value.h"

typedef struct {
	uint32_t op;
	uint32_t cnt;
	cff_Value *vals;
} cff_DictEntry;

typedef struct {
	uint32_t count;
	cff_DictEntry *ents;
} cff_Dict;

extern caryll_ElementInterfaceOf(cff_Dict) {
	caryll_RT(cff_Dict);
	cff_Dict *(*parse)(const uint8_t *data, const uint32_t len);
	void (*parseToCallback)(const uint8_t *data, const uint32_t len, void *context,
	                        void (*callback)(uint32_t op, uint8_t top, cff_Value * stack, void *context));
	cff_Value (*parseDictKey)(const uint8_t *data, const uint32_t len, const uint32_t op, const uint32_t idx);
	caryll_Buffer *(*build)(const cff_Dict *dict);
}
cff_iDict;

#endif
