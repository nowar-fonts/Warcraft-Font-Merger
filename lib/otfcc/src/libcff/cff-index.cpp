#include "cff-index.h"
// INDEX util functions

static INLINE void disposeCffIndex(cff_Index *in) {
	if (in->offset) FREE(in->offset);
	if (in->data) FREE(in->data);
}

caryll_standardRefTypeFn(cff_Index, disposeCffIndex);

static uint32_t getIndexLength(const cff_Index *i) {
	if (i->count != 0)
		return 3 + (i->offset[i->count] - 1) + ((i->count + 1) * i->offSize);
	else
		return 3;
}
static void emptyIndex(cff_Index *i) {
	cff_iIndex.dispose(i);
	memset(i, 0, sizeof(*i));
}

static void extractIndex(uint8_t *data, uint32_t pos, cff_Index *in) {
	in->count = gu2(data, pos);
	in->offSize = gu1(data, pos + 2);

	if (in->count > 0) {
		NEW(in->offset, in->count + 1);

		for (arity_t i = 0; i <= in->count; i++) {
			switch (in->offSize) {
				case 1:
					in->offset[i] = gu1(data, pos + 3 + (i * in->offSize));
					break;
				case 2:
					in->offset[i] = gu2(data, pos + 3 + (i * in->offSize));
					break;
				case 3:
					in->offset[i] = gu3(data, pos + 3 + (i * in->offSize));
					break;
				case 4:
					in->offset[i] = gu4(data, pos + 3 + (i * in->offSize));
					break;
			}
		}

		NEW(in->data, in->offset[in->count] - 1);
		memcpy(in->data, data + pos + 3 + (in->count + 1) * in->offSize, in->offset[in->count] - 1);
	} else {
		in->offset = NULL;
		in->data = NULL;
	}
}

static cff_Index *newIndexByCallback(void *context, uint32_t length, caryll_Buffer *(*fn)(void *, uint32_t)) {
	cff_Index *idx = cff_iIndex.create();
	idx->count = length;
	NEW(idx->offset, idx->count + 1);
	idx->offset[0] = 1;
	idx->data = NULL;

	size_t used = 0;
	size_t blank = 0;
	for (arity_t i = 0; i < length; i++) {
		caryll_Buffer *blob = fn(context, i);
		if (blank < blob->size) {
			used += blob->size;
			blank = (used >> 1) & 0xFFFFFF;
			RESIZE(idx->data, used + blank);
		} else {
			used += blob->size;
			blank -= blob->size;
		}
		idx->offset[i + 1] = (uint32_t)(blob->size + idx->offset[i]);
		memcpy(idx->data + idx->offset[i] - 1, blob->data, blob->size);
		buffree(blob);
	}
	idx->offSize = 4;
	return idx;
}

static caryll_Buffer *buildIndex(const cff_Index *index) {
	caryll_Buffer *blob = bufnew();
	if (!index->count) {
		bufwrite8(blob, 0);
		bufwrite8(blob, 0);
		bufwrite8(blob, 0);
		return blob;
	}

	uint32_t lastOffset = index->offset[index->count];
	uint8_t offSize = 4;
	if (lastOffset < 0x100) {
		offSize = 1;
	} else if (lastOffset < 0x10000) {
		offSize = 2;
	} else if (lastOffset < 0x1000000) {
		offSize = 3;
	} else {
		offSize = 4;
	}

	if (index->count != 0)
		blob->size = 3 + (index->offset[index->count] - 1) + ((index->count + 1) * offSize);
	else
		blob->size = 3;

	NEW(blob->data, blob->size);
	blob->data[0] = index->count / 256;
	blob->data[1] = index->count % 256;
	blob->data[2] = offSize;

	if (index->count > 0) {
		for (arity_t i = 0; i <= index->count; i++) {
			switch (offSize) {
				case 1:
					blob->data[3 + i] = index->offset[i];
					break;
				case 2:
					blob->data[3 + i * 2] = index->offset[i] / 256;
					blob->data[4 + i * 2] = index->offset[i] % 256;
					break;
				case 3:
					blob->data[3 + i * 3] = index->offset[i] / 65536;
					blob->data[4 + i * 3] = (index->offset[i] % 65536) / 256;
					blob->data[5 + i * 3] = (index->offset[i] % 65536) % 256;
					break;
				case 4:
					blob->data[3 + i * 4] = (index->offset[i] / 65536) / 256;
					blob->data[4 + i * 4] = (index->offset[i] / 65536) % 256;
					blob->data[5 + i * 4] = (index->offset[i] % 65536) / 256;
					blob->data[6 + i * 4] = (index->offset[i] % 65536) % 256;
					break;
			}
		}

		if (index->data != NULL)
			memcpy(blob->data + 3 + ((index->count + 1) * offSize), index->data, index->offset[index->count] - 1);
	}
	blob->cursor = blob->size;
	return blob;
}

caryll_ElementInterfaceOf(cff_Index) cff_iIndex = {
    caryll_standardRefTypeMethods(cff_Index), .getLength = getIndexLength, .empty = emptyIndex, .parse = extractIndex,
    .fromCallback = newIndexByCallback,       .build = buildIndex,
};
