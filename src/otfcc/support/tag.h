#ifndef CARYLL_SUPPORT_TAG_H
#define CARYLL_SUPPORT_TAG_H

#include <stdint.h>
#include <stdlib.h>

#ifndef INLINE
#ifdef _MSC_VER
#define INLINE __forceinline /* use __forceinline (VC++ specific) */
#else
#define INLINE inline /* use standard inline */
#endif
#endif

// Tag handler
static INLINE void tag2str(uint32_t tag, char tags[4]) {
	tags[0] = (tag >> 24) & 0xFF;
	tags[1] = (tag >> 16) & 0xFF;
	tags[2] = (tag >> 8) & 0xFF;
	tags[3] = tag & 0xFF;
}

static INLINE uint32_t str2tag(const char *tags) {
	if (!tags) return 0;
	uint32_t tag = 0;
	uint8_t len = 0;
	while (*tags && len < 4) {
		tag = (tag << 8) | (*tags), tags++, len++;
	}
	while (len < 4) {
		tag = (tag << 8) | ' ', len++;
	}
	return tag;
}

#endif
