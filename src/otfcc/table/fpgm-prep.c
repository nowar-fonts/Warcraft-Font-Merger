#include "fpgm-prep.h"

#include "support/util.h"
#include "support/ttinstr/ttinstr.h"

static INLINE void disposeFpgmPrep(MOVE table_fpgm_prep *table) {
	if (table->tag) sdsfree(table->tag);
	if (table->bytes) FREE(table->bytes);
}

caryll_standardRefType(table_fpgm_prep, table_iFpgm_prep, disposeFpgmPrep);

table_fpgm_prep *otfcc_readFpgmPrep(const otfcc_Packet packet, const otfcc_Options *options,
                                    uint32_t tag) {
	table_fpgm_prep *t = NULL;
	FOR_TABLE(tag, table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;
		t = table_iFpgm_prep.create();
		t->tag = NULL;
		t->length = length;
		NEW(t->bytes, length);
		if (!t->bytes) goto FAIL;
		memcpy(t->bytes, data, length);
		return t;
	FAIL:
		table_iFpgm_prep.free(t);
		t = NULL;
	}
	return NULL;
}

void table_dumpTableFpgmPrep(const table_fpgm_prep *table, json_value *root,
                             const otfcc_Options *options, const char *tag) {
	if (!table) return;
	loggedStep("%s", tag) {
		json_object_push(root, tag, dump_ttinstr(table->bytes, table->length, options));
	}
}

void makeFpgmPrepInstr(void *_t, uint8_t *instrs, uint32_t length) {
	table_fpgm_prep *t = (table_fpgm_prep *)_t;
	t->length = length;
	t->bytes = instrs;
}
void wrongFpgmPrepInstr(void *_t, char *reason, int pos) {
	/*
	table_fpgm_prep *t = (table_fpgm_prep *)_t;
	fprintf(stderr, "[OTFCC] TrueType instructions parse error : %s, at %d in /%s\n", reason, pos,
	t->tag);
	*/
}

table_fpgm_prep *otfcc_parseFpgmPrep(const json_value *root, const otfcc_Options *options,
                                     const char *tag) {
	table_fpgm_prep *t = NULL;
	json_value *table = NULL;
	if ((table = json_obj_get(root, tag))) {
		loggedStep("%s", tag) {
			t = table_iFpgm_prep.create();
			t->tag = sdsnew(tag);
			parse_ttinstr(table, t, makeFpgmPrepInstr, wrongFpgmPrepInstr);
		}
	}
	return t;
}

caryll_Buffer *otfcc_buildFpgmPrep(const table_fpgm_prep *table, const otfcc_Options *options) {
	if (!table) return NULL;
	caryll_Buffer *buf = bufnew();
	bufwrite_bytes(buf, table->length, table->bytes);
	return buf;
}
