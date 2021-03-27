#include "hdmx.h"

#include "support/util.h"

static INLINE void disposeHdmx(MOVE table_hdmx *table) {
	if (!table->records) return;
	for (uint32_t i = 0; i < table->numRecords; i++) {
		if (table->records[i].widths != NULL) FREE(table->records[i].widths);
	}
	FREE(table->records);
}

caryll_standardRefType(table_hdmx, table_iHdmx, disposeHdmx);

table_hdmx *otfcc_readHdmx(otfcc_Packet packet, const otfcc_Options *options, table_maxp *maxp) {
	FOR_TABLE('hdmx', table) {
		font_file_pointer data = table.data;

		table_hdmx *hdmx;
		NEW(hdmx);
		hdmx->version = read_16u(data);
		hdmx->numRecords = read_16u(data + 2);
		hdmx->sizeDeviceRecord = read_32u(data + 4);
		NEW(hdmx->records, hdmx->numRecords);

		for (uint32_t i = 0; i < hdmx->numRecords; i++) {
			hdmx->records[i].pixelSize = *(data + 8 + i * (2 + maxp->numGlyphs));
			hdmx->records[i].maxWidth = *(data + 8 + i * (2 + maxp->numGlyphs) + 1);
			NEW(hdmx->records[i].widths, maxp->numGlyphs);
			memcpy(hdmx->records[i].widths, data + 8 + i * (2 + maxp->numGlyphs) + 2, maxp->numGlyphs);
		}

		return hdmx;
	}
	return NULL;
}
