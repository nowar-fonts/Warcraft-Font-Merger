#include "../meta.h"

#include "support/util.h"
#include "bk/bkgraph.h"

caryll_Buffer *otfcc_buildMeta(const table_meta *meta, const otfcc_Options *options) {
	if (!meta || !meta->entries.length) return NULL;
	bk_Block *root = bk_new_Block(b32, meta->version,                  // Version
	                              b32, meta->flags,                    // Flags
	                              b32, 0,                              // RESERVED
	                              b32, (uint32_t)meta->entries.length, // dataMapsCount
	                              bkover);
	foreach (meta_Entry *e, meta->entries) {
		bk_push(root,                                                    /// begin
		        b32, e->tag,                                             // tag
		        p32, bk_newBlockFromStringLen(sdslen(e->data), e->data), // dataOffset
		        b32, sdslen(e->data),                                    // dataLength
		        bkover);
	}
	return bk_build_Block(root);
}
