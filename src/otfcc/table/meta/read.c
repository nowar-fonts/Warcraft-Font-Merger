#include "../meta.h"

#include "support/util.h"

table_meta *otfcc_readMeta(const otfcc_Packet packet, const otfcc_Options *options) {
	table_meta *meta = NULL;
	FOR_TABLE('meta', table) {
		if (table.length < 16) goto FAIL;
		uint32_t version = read_32u(table.data + 0);
		uint32_t flags = read_32u(table.data + 4);
		uint32_t dataMapsCount = read_32u(table.data + 12);
		if (table.length < 16 + 12 * dataMapsCount) goto FAIL;

		meta = table_iMeta.create();
		meta->version = version;
		meta->flags = flags;

		for (uint32_t j = 0; j < dataMapsCount; j++) {
			uint32_t tag = read_32u(table.data + 16 + 12 * j + 0);
			uint32_t offset = read_32u(table.data + 16 + 12 * j + 4);
			uint32_t length = read_32u(table.data + 16 + 12 * j + 8);
			if (table.length < offset + length) continue;

			meta_iEntries.push(
			    &meta->entries,
			    (meta_Entry){.tag = tag, .data = sdsnewlen((char *)(table.data + offset), length)});
		}
		return meta;

	FAIL:
		logWarning("Table 'meta' corrupted.\n");
		table_iMeta.free(meta);
		meta = NULL;
	}
	return meta;
}
