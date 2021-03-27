#include "../meta.h"

#include "support/util.h"

static void initMetaEntry(meta_Entry *e) {
	e->tag = 1;
	e->data = NULL;
}

static void disposeMetaEntry(meta_Entry *e) {
	sdsfree(e->data);
}

caryll_standardType(meta_Entry, meta_iEntry, initMetaEntry, disposeMetaEntry);
caryll_standardVectorImpl(meta_Entries, meta_Entry, meta_iEntry, meta_iEntries);

static void initMetaTable(table_meta *t) {
	t->version = 1;
	t->flags = 0;
	meta_iEntries.init(&t->entries);
}

static void disposeMetaTable(table_meta *t) {
	meta_iEntries.dispose(&t->entries);
}

caryll_standardRefType(table_meta, table_iMeta, initMetaTable, disposeMetaTable);
