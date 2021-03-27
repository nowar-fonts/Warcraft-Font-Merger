#ifndef CARYLL_cff_SUBR_H
#define CARYLL_cff_SUBR_H

#include "libcff.h"
#include "charstring-il.h"

typedef struct __cff_SubrRule cff_SubrRule;
typedef struct __cff_SubrNode cff_SubrNode;

struct __cff_SubrNode {
	cff_SubrNode *prev;
	cff_SubrRule *rule;
	OWNING cff_SubrNode *next;
	OWNING caryll_Buffer *terminal;
	bool hard;
	bool guard;
	bool last;
};

struct __cff_SubrRule {
	bool printed;
	bool numbered;
	uint32_t number;
	uint32_t height;
	uint32_t uniqueIndex;
	uint16_t cffIndex;
	uint32_t refcount;
	uint32_t effectiveLength;
	OWNING cff_SubrNode *guard;
	OWNING cff_SubrRule *next;
};

typedef struct {
	uint8_t arity;
	uint8_t *key;
	cff_SubrNode *start;
	UT_hash_handle hh;
} cff_SubrDiagramIndex;

typedef struct {
	OWNING cff_SubrRule *root;
	cff_SubrRule *last;
	cff_SubrDiagramIndex *diagramIndex;
	uint32_t totalRules;
	uint32_t totalCharStrings;
	bool doSubroutinize;
} cff_SubrGraph;

extern caryll_RefElementInterface(cff_SubrGraph) cff_iSubrGraph;

void cff_insertILToGraph(cff_SubrGraph *g, cff_CharstringIL *il);
void cff_ilGraphToBuffers(cff_SubrGraph *g, caryll_Buffer **s, caryll_Buffer **gs, caryll_Buffer **ls,
                          const otfcc_Options *options);

#endif
