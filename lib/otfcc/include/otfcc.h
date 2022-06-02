#pragma once

#include <stdbool.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

struct otfcc_option {
	bool pretty;
};

bool otfcc_dump(const char *font_buffer, size_t font_length, const otfcc_option *option,
                char **p_json_buffer, size_t *p_json_length, int *p_error);

void otfcc_free_buffer(char *buffer);

#ifdef __cplusplus
}
#endif
