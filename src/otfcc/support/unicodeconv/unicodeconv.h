#ifndef CARYLL_SUPPORT_UNICODECONV_H
#define CARYLL_SUPPORT_UNICODECONV_H
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include "dep/sds.h"
sds utf16le_to_utf8(const uint8_t *inb, int inlenb);
sds utf16be_to_utf8(const uint8_t *inb, int inlenb);
uint8_t *utf8toutf16be(sds _in, size_t *out_bytes);
#endif
