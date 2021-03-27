#ifndef CARYLL_SUPPORT_OTFCC_ALLOC_H
#define CARYLL_SUPPORT_OTFCC_ALLOC_H

#include <stdio.h>
#include <stdlib.h>

#ifndef INLINE
#ifdef _MSC_VER
#define INLINE __forceinline /* use __forceinline (VC++ specific) */
#else
#define INLINE inline /* use standard inline */
#endif
#endif

// Allocators
// Change this if you prefer other allocators
#define __caryll_malloc malloc
#define __caryll_calloc calloc
#define __caryll_realloc realloc
#define __caryll_free free

static INLINE void *__caryll_allocate_dirty(size_t n, unsigned long line) {
	if (!n) return NULL;
	void *p = __caryll_malloc(n);
	if (!p) {
		fprintf(stderr, "[%ld]Out of memory(%ld bytes)\n", line, (unsigned long)n);
		exit(EXIT_FAILURE);
	}
	return p;
}
static INLINE void *__caryll_allocate_clean(size_t n, unsigned long line) {
	if (!n) return NULL;
	void *p = __caryll_calloc(n, 1);
	if (!p) {
		fprintf(stderr, "[%ld]Out of memory(%ld bytes)\n", line, (unsigned long)n);
		exit(EXIT_FAILURE);
	}
	return p;
}
static INLINE void *__caryll_reallocate(void *ptr, size_t n, unsigned long line) {
	if (!n) {
		__caryll_free(ptr);
		return NULL;
	}
	if (!ptr) {
		return __caryll_allocate_clean(n, line);
	} else {
		void *p = __caryll_realloc(ptr, n);
		if (!p) {
			fprintf(stderr, "[%ld]Out of memory(%ld bytes)\n", line, (unsigned long)n);
			exit(EXIT_FAILURE);
		}
		return p;
	}
}
#ifdef __cplusplus
#define NEW_CLEAN_S(ptr, size) ptr = __caryll_allocate_clean((size), __LINE__)
#define NEW_CLEAN_1(ptr)                                                                           \
	ptr = (decltype(ptr))__caryll_allocate_clean(sizeof(decltype(*ptr)), __LINE__)
#define NEW_CLEAN_N(ptr, n)                                                                        \
	ptr = (decltype(ptr))__caryll_allocate_clean(sizeof(decltype(*ptr)) * (n), __LINE__)
#define NEW_DIRTY(ptr)                                                                             \
	ptr = (decltype(ptr))__caryll_allocate_dirty(sizeof(decltype(*ptr)), __LINE__)
#define NEW_DIRTY_N(ptr, n)                                                                        \
	ptr = (decltype(ptr))__caryll_allocate_dirty(sizeof(decltype(*ptr)) * (n), __LINE__)
#define FREE(ptr) (__caryll_free(ptr), ptr = nullptr)
#define DELETE(fn, ptr) (fn(ptr), ptr = nullptr)
#define RESIZE(ptr, n) ptr = (decltype(ptr))__caryll_reallocate(ptr, sizeof(*ptr) * (n), __LINE__)
#else
#define NEW_CLEAN_S(ptr, size) ptr = __caryll_allocate_clean((size), __LINE__)
#define NEW_CLEAN_1(ptr) ptr = __caryll_allocate_clean(sizeof(*ptr), __LINE__)
#define NEW_CLEAN_N(ptr, n) ptr = __caryll_allocate_clean(sizeof(*ptr) * (n), __LINE__)
#define NEW_DIRTY(ptr) ptr = __caryll_allocate_dirty(sizeof(*ptr), __LINE__)
#define NEW_DIRTY_N(ptr, n) ptr = __caryll_allocate_dirty(sizeof(*ptr) * (n), __LINE__)
#define FREE(ptr) (__caryll_free(ptr), ptr = NULL)
#define DELETE(fn, ptr) (fn(ptr), ptr = NULL)
#define RESIZE(ptr, n) ptr = __caryll_reallocate(ptr, sizeof(*ptr) * (n), __LINE__)
#endif

#define __GET_MACRO_OTFCC_ALLOC_2(_1, _2, NAME, ...) NAME
#define NEW(...) __GET_MACRO_OTFCC_ALLOC_2(__VA_ARGS__, NEW_CLEAN_N, NEW_CLEAN_1)(__VA_ARGS__)

#endif
