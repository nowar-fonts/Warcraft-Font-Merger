#include "cff-fdselect.h"

void cff_close_FDSelect(cff_FDSelect fds) {
	switch (fds.t) {
		case cff_FDSELECT_FORMAT0:
			if (fds.f0.fds != NULL) FREE(fds.f0.fds);
			break;
		case cff_FDSELECT_FORMAT3:
			if (fds.f3.range3 != NULL) FREE(fds.f3.range3);
			break;
		case cff_FDSELECT_UNSPECED:
			break;
	}
}

caryll_Buffer *cff_build_FDSelect(cff_FDSelect fd) {
	switch (fd.t) {
		case cff_FDSELECT_UNSPECED: {
			return bufnew();
		}
		case cff_FDSELECT_FORMAT0: {
			caryll_Buffer *blob = bufnew();
			blob->size = 1 + fd.s;
			NEW(blob->data, blob->size);
			for (uint16_t j = 0; j < fd.s; j++) {
				blob->data[j] = fd.f0.fds[j];
			}
			return blob;
		}
		case cff_FDSELECT_FORMAT3: {
			caryll_Buffer *blob = bufnew();
			blob->size = 5 + fd.f3.nranges * 3;
			NEW(blob->data, blob->size);
			blob->data[0] = 3;
			blob->data[1] = fd.f3.nranges / 256;
			blob->data[2] = fd.f3.nranges % 256;
			for (int i = 0; i < fd.f3.nranges; i++)
				blob->data[3 + 3 * i] = fd.f3.range3[i].first / 256,
				                   blob->data[4 + 3 * i] = fd.f3.range3[i].first % 256,
				                   blob->data[5 + 3 * i] = fd.f3.range3[i].fd;
			blob->data[blob->size - 2] = fd.f3.sentinel / 256;
			blob->data[blob->size - 1] = fd.f3.sentinel % 256;
			return blob;
		}
		default: { return NULL; }
	}
}

void cff_extract_FDSelect(uint8_t *data, int32_t offset, uint16_t nchars, cff_FDSelect *fdselect) {
	switch (data[offset]) {
		case 0: {
			fdselect->t = cff_FDSELECT_FORMAT0;
			fdselect->f0.format = 0;
			fdselect->s = nchars;
			NEW(fdselect->f0.fds, nchars);

			for (uint32_t i = 0; i < nchars; i++) {
				fdselect->f0.fds[i] = gu1(data, offset + 1 + i);
			}
			break;
		}
		case 3: {
			fdselect->t = cff_FDSELECT_FORMAT3;
			fdselect->f3.format = 3;
			fdselect->f3.nranges = gu2(data, offset + 1);
			NEW(fdselect->f3.range3, fdselect->f3.nranges);

			for (uint32_t i = 0; i < fdselect->f3.nranges; i++) {
				fdselect->f3.range3[i].first = gu2(data, offset + 3 + i * 3);
				fdselect->f3.range3[i].fd = gu1(data, offset + 5 + i * 3);
			}

			fdselect->f3.sentinel = gu2(data, offset + (fdselect->f3.nranges + 1) * 3);
			break;
		}
		default: {
			fdselect->t = cff_FDSELECT_UNSPECED;
			break;
		}
	}
}
