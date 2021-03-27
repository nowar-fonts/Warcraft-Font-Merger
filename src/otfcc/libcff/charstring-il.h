#ifndef CARYLL_cff_CHARSTRING_IL
#define CARYLL_cff_CHARSTRING_IL

#include "libcff.h"
#include "otfcc/table/glyf.h"

typedef enum {
	IL_ITEM_OPERAND,
	IL_ITEM_OPERATOR,
	IL_ITEM_SPECIAL,
	IL_ITEM_PHANTOM_OPERATOR,
	IL_ITEM_PHANTOM_OPERAND
} cff_InstructionType;

typedef struct {
	cff_InstructionType type;
	arity_t arity;
	union {
		double d;  // for type == IL_ITEM_OPERAND, IL_ITEM_PHANTOM_OPERAND
		int32_t i; // otherwise
	};
} cff_CharstringInstruction;

typedef struct {
	uint32_t length;
	uint32_t free;
	cff_CharstringInstruction *instr;
} cff_CharstringIL;

bool instruction_eq(cff_CharstringInstruction *z1, cff_CharstringInstruction *z2);

// basic ops
cff_CharstringIL *cff_compileGlyphToIL(glyf_Glyph *g, uint16_t defaultWidth, uint16_t nominalWidth);
void cff_optimizeIL(cff_CharstringIL *il, const otfcc_Options *options);
cff_CharstringIL *cff_shrinkIL(cff_CharstringIL *il);
void cff_ILmergeIL(cff_CharstringIL *self, cff_CharstringIL *il);
caryll_Buffer *cff_build_IL(cff_CharstringIL *il);
bool cff_ilEqual(cff_CharstringIL *a, cff_CharstringIL *b);

void il_push_operand(cff_CharstringIL *il, double x);
void il_push_op(cff_CharstringIL *il, int32_t op);
void il_push_special(cff_CharstringIL *il, int32_t s);

#endif
