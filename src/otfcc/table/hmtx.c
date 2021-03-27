#include "hmtx.h"

#include "support/util.h"

static INLINE void disposeHmtx(MOVE table_hmtx *table) {
	if (table->metrics != NULL) FREE(table->metrics);
	if (table->leftSideBearing != NULL) FREE(table->leftSideBearing);
}

caryll_standardRefType(table_hmtx, table_iHmtx, disposeHmtx);

table_hmtx *otfcc_readHmtx(const otfcc_Packet packet, const otfcc_Options *options,
                           table_hhea *hhea, table_maxp *maxp) {
	if (!hhea || !maxp || !hhea->numberOfMetrics || maxp->numGlyphs < hhea->numberOfMetrics) {
		return NULL;
	}
	FOR_TABLE('hmtx', table) {
		font_file_pointer data = table.data;
		uint32_t length = table.length;

		table_hmtx *hmtx = NULL;

		glyphid_t count_a = hhea->numberOfMetrics;
		glyphid_t count_k = maxp->numGlyphs - hhea->numberOfMetrics;
		if (length < count_a * 4 + count_k * 2) goto HMTX_CORRUPTED;

		NEW(hmtx);
		NEW(hmtx->metrics, count_a);
		NEW(hmtx->leftSideBearing, count_k);

		for (glyphid_t ia = 0; ia < count_a; ia++) {
			hmtx->metrics[ia].advanceWidth = read_16u(data + ia * 4);
			hmtx->metrics[ia].lsb = read_16s(data + ia * 4 + 2);
		}

		for (glyphid_t ik = 0; ik < count_k; ik++) {
			hmtx->leftSideBearing[ik] = read_16s(data + count_a * 4 + ik * 2);
		}

		return hmtx;
	HMTX_CORRUPTED:
		logWarning("Table 'hmtx' corrupted.\n");
		if (hmtx) { table_iHmtx.free(hmtx), hmtx = NULL; }
	}
	return NULL;
}

caryll_Buffer *otfcc_buildHmtx(const table_hmtx *hmtx, glyphid_t count_a, glyphid_t count_k,
                               const otfcc_Options *options) {
	caryll_Buffer *buf = bufnew();
	if (!hmtx) return buf;
	if (hmtx->metrics) {
		for (glyphid_t j = 0; j < count_a; j++) {
			bufwrite16b(buf, hmtx->metrics[j].advanceWidth);
			bufwrite16b(buf, hmtx->metrics[j].lsb);
		}
	}
	if (hmtx->leftSideBearing) {
		for (glyphid_t j = 0; j < count_k; j++) {
			bufwrite16b(buf, hmtx->leftSideBearing[j]);
		}
	}
	return buf;
}
