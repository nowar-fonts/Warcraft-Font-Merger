#ifndef CARYLL_SUPPORT_VECTOR_IMPL_H
#define CARYLL_SUPPORT_VECTOR_IMPL_H

#include "caryll/vector.h"
#include "element-impl.h"

#ifndef __CARYLL_INLINE__
#ifdef _MSC_VER
#define __CARYLL_INLINE__ __forceinline /* use __forceinline (VC++ specific) */
#else
#define __CARYLL_INLINE__ inline /* use standard inline */
#endif
#endif

#if (__GNUC__ > 2) || (__GNUC__ == 2 && __GNUC_MINOR__ > 4)
#define __CARYLL_MAY_UNUSED__ __attribute__((__unused__))
#elif (__clang_major__ > 2)
#define __CARYLL_MAY_UNUSED__ __attribute__((__unused__))
#else
#define __CARYLL_MAY_UNUSED__
#endif

#define __CARYLL_VECTOR_INITIAL_SIZE 2

#define caryll_VectorImplFreeIndependent(__TV, __T, __ti)                                          \
	static __CARYLL_INLINE__ void __TV##_dispose(__TV *arr) {                                      \
		if (!arr) return;                                                                          \
		if ((__ti).dispose) {                                                                      \
			for (size_t j = arr->length; j--;) {                                                   \
				(__ti).dispose(&arr->items[j]);                                                    \
			}                                                                                      \
		}                                                                                          \
		__caryll_free(arr->items);                                                                 \
		arr->items = NULL;                                                                         \
		arr->length = 0;                                                                           \
		arr->capacity = 0;                                                                         \
	}

#define caryll_VectorImplFreeDependent(__TV, __T, __TX, fn)                                        \
	static __CARYLL_INLINE__ void __TV##_disposeDependent(__TV *arr, const __TX *enclosure) {      \
		if (!arr) return;                                                                          \
		for (size_t j = arr->length; j--;) {                                                       \
			fn(&arr->items[j], enclosure);                                                         \
		}                                                                                          \
		__caryll_free(arr->items);                                                                 \
		arr->items = NULL;                                                                         \
		arr->length = 0;                                                                           \
		arr->capacity = 0;                                                                         \
	}

#define caryll_VectorImplFunctionsCommon(__TV, __T, __ti)                                          \
	static __CARYLL_INLINE__ void __TV##_init(MODIFY __TV *arr) {                                  \
		arr->length = 0;                                                                           \
		arr->capacity = 0;                                                                         \
		arr->items = NULL;                                                                         \
	}                                                                                              \
	caryll_trivialCreate(__TV);                                                                    \
	caryll_trivialFree(__TV);                                                                      \
	static __CARYLL_INLINE__ void __TV##_growTo(MODIFY __TV *arr, size_t target) {                 \
		if (target <= arr->capacity) return;                                                       \
		if (arr->capacity < __CARYLL_VECTOR_INITIAL_SIZE)                                          \
			arr->capacity = __CARYLL_VECTOR_INITIAL_SIZE;                                          \
		while (arr->capacity < target) {                                                           \
			arr->capacity += arr->capacity / 2;                                                    \
		}                                                                                          \
		if (arr->items) {                                                                          \
			arr->items = __caryll_realloc(arr->items, arr->capacity * sizeof(__T));                \
		} else {                                                                                   \
			arr->items = __caryll_calloc(arr->capacity, sizeof(__T));                              \
		}                                                                                          \
	}                                                                                              \
	static __CARYLL_INLINE__ void __TV##_growToN(MODIFY __TV *arr, size_t target) {                \
		if (target <= arr->capacity) return;                                                       \
		if (arr->capacity < __CARYLL_VECTOR_INITIAL_SIZE)                                          \
			arr->capacity = __CARYLL_VECTOR_INITIAL_SIZE;                                          \
		if (arr->capacity < target) { arr->capacity = target + 1; }                                \
		if (arr->items) {                                                                          \
			arr->items = __caryll_realloc(arr->items, arr->capacity * sizeof(__T));                \
		} else {                                                                                   \
			arr->items = __caryll_calloc(arr->capacity, sizeof(__T));                              \
		}                                                                                          \
	}                                                                                              \
	static __CARYLL_INLINE__ void __TV##_resizeTo(MODIFY __TV *arr, size_t target) {               \
		arr->capacity = target;                                                                    \
		if (arr->items) {                                                                          \
			arr->items = __caryll_realloc(arr->items, arr->capacity * sizeof(__T));                \
		} else {                                                                                   \
			arr->items = __caryll_calloc(arr->capacity, sizeof(__T));                              \
		}                                                                                          \
	}                                                                                              \
	static __CARYLL_INLINE__ void __TV##_shrinkToFit(MODIFY __TV *arr) {                           \
		__TV##_resizeTo(arr, arr->length);                                                         \
	}                                                                                              \
	static __CARYLL_INLINE__ void __TV##_grow(MODIFY __TV *arr) {                                  \
		__TV##_growTo(arr, arr->length + 1);                                                       \
	}                                                                                              \
	static __CARYLL_INLINE__ void __TV##_push(MODIFY __TV *arr, MOVE __T elem) {                   \
		__TV##_grow(arr);                                                                          \
		(arr)->items[(arr)->length++] = (elem);                                                    \
	}                                                                                              \
	static __CARYLL_INLINE__ __T __TV##_pop(MODIFY __TV *arr) {                                    \
		__T t = arr->items[arr->length - 1];                                                       \
		arr->length -= 1;                                                                          \
		return t;                                                                                  \
	}                                                                                              \
	static __CARYLL_INLINE__ void __TV##_fill(MODIFY __TV *arr, size_t n) {                        \
		while (arr->length < n) {                                                                  \
			__T x;                                                                                 \
			if ((__ti).init) {                                                                     \
				(__ti).init(&x);                                                                   \
			} else {                                                                               \
				memset(&x, 0, sizeof(x));                                                          \
			}                                                                                      \
			__TV##_push(arr, x);                                                                   \
		}                                                                                          \
	}                                                                                              \
	static __CARYLL_INLINE__ void __TV##_initN(MODIFY __TV *arr, size_t n) {                       \
		__TV##_init(arr);                                                                          \
		__TV##_growToN(arr, n);                                                                    \
		__TV##_fill(arr, n);                                                                       \
	}                                                                                              \
	static __CARYLL_INLINE__ void __TV##_initCapN(MODIFY __TV *arr, size_t n) {                    \
		__TV##_init(arr);                                                                          \
		__TV##_growToN(arr, n);                                                                    \
	}                                                                                              \
	static __CARYLL_INLINE__ __TV *__TV##_createN(size_t n) {                                      \
		__TV *t = __caryll_malloc(sizeof(__TV));                                                   \
		__TV##_initN(t, n);                                                                        \
		return t;                                                                                  \
	}                                                                                              \
	static __CARYLL_INLINE__ void __TV##_move(MODIFY __TV *dst, MOVE __TV *src) {                  \
		*dst = *src;                                                                               \
		__TV##_init(src);                                                                          \
	}                                                                                              \
	static __CARYLL_INLINE__ void __TV##_copy(MODIFY __TV *dst, const __TV *src) {                 \
		__TV##_init(dst);                                                                          \
		__TV##_growTo(dst, src->length);                                                           \
		dst->length = src->length;                                                                 \
		if ((__ti).copy) {                                                                         \
			for (size_t j = 0; j < src->length; j++) {                                             \
				(__ti).copy(&dst->items[j], (const __T *)&src->items[j]);                          \
			}                                                                                      \
		} else {                                                                                   \
			for (size_t j = 0; j < src->length; j++) {                                             \
				dst->items[j] = src->items[j];                                                     \
			}                                                                                      \
		}                                                                                          \
	}                                                                                              \
	static __CARYLL_INLINE__ void __TV##_sort(MODIFY __TV *arr,                                    \
	                                          int (*fn)(const __T *a, const __T *b)) {             \
		qsort(arr->items, arr->length, sizeof(arr->items[0]),                                      \
		      (int (*)(const void *, const void *))fn);                                            \
	}                                                                                              \
	static __CARYLL_INLINE__ void __TV##_filterEnv(                                                \
	    MODIFY __TV *arr, bool (*fn)(const __T *a, void *env), void *env) {                        \
		size_t j = 0;                                                                              \
		for (size_t k = 0; k < arr->length; k++) {                                                 \
			if (fn(&arr->items[k], env)) {                                                         \
				if (j != k) arr->items[j] = arr->items[k];                                         \
				j++;                                                                               \
			} else {                                                                               \
				(__ti).dispose ? (__ti).dispose(&((arr)->items[k])) : (void)0;                     \
			}                                                                                      \
		};                                                                                         \
		arr->length = j;                                                                           \
	}                                                                                              \
	static __CARYLL_INLINE__ void __TV##_disposeItem(MODIFY __TV *arr, size_t n) {                 \
		(__ti).dispose ? (__ti).dispose(&((arr)->items[n])) : (void)0;                             \
	}                                                                                              \
	caryll_trivialReplace(__TV);

#define caryll_VectorImplFunctions(__TV, __T, __ti)                                                \
	caryll_VectorImplFreeIndependent(__TV, __T, __ti);                                             \
	caryll_VectorImplFunctionsCommon(__TV, __T, __ti);

#define caryll_VectorImplAssignments(__TV, __T, __ti)                                              \
	.init = __TV##_init, .copy = __TV##_copy, .dispose = __TV##_dispose, .create = __TV##_create,  \
	.createN = __TV##_createN, .free = __TV##_free, .initN = __TV##_initN,                         \
	.initCapN = __TV##_initCapN, .clear = __TV##_dispose, .replace = __TV##_replace,               \
	.copyReplace = __TV##_copyReplace, .push = __TV##_push, .pop = __TV##_pop,                     \
	.fill = __TV##_fill, .sort = __TV##_sort, .disposeItem = __TV##_disposeItem,                   \
	.filterEnv = __TV##_filterEnv, .move = __TV##_move, .shrinkToFit = __TV##_shrinkToFit

#define caryll_standardVectorImpl(__TV, __T, __ti, __name)                                         \
	caryll_VectorImplFunctions(__TV, __T, __ti);                                                   \
	caryll_VectorInterfaceTypeName(__TV) __name = {                                                \
	    caryll_VectorImplAssignments(__TV, __T, __ti),                                             \
	};

#endif
