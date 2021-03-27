#ifndef CARYLL_SUPPORT_BASE64_H
#define CARYLL_SUPPORT_BASE64_H

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
uint8_t *base64_encode(const uint8_t *src, size_t len, size_t *out_len);
uint8_t *base64_decode(const uint8_t *src, size_t len, size_t *out_len);

#endif
