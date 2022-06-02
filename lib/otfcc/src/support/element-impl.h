#ifndef CARYLL_SUPPORT_ELEMENT_IMPL_H
#define CARYLL_SUPPORT_ELEMENT_IMPL_H

#include "caryll/element.h"
#include "otfcc-alloc.h"

#ifndef __CARYLL_INLINE__
#ifdef _MSC_VER
#define __CARYLL_INLINE__ __forceinline /* use __forceinline (VC++ specific) */
#else
#define __CARYLL_INLINE__ inline /* use standard inline */
#endif
#endif

#define __NARG__(...) __NARG_I_(__VA_ARGS__, __RSEQ_N())
#define __NARG_I_(...) __ARG_N(__VA_ARGS__)
#define __ARG_N(_1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12, _13, _14, _15, _16, _17, _18,   \
                _19, _20, _21, _22, _23, _24, _25, _26, _27, _28, _29, _30, _31, _32, _33, _34,    \
                _35, _36, _37, _38, _39, _40, _41, _42, _43, _44, _45, _46, _47, _48, _49, _50,    \
                _51, _52, _53, _54, _55, _56, _57, _58, _59, _60, _61, _62, _63, N, ...)           \
	N
#define __RSEQ_N()                                                                                 \
	63, 62, 61, 60, 59, 58, 57, 56, 55, 54, 53, 52, 51, 50, 49, 48, 47, 46, 45, 44, 43, 42, 41,    \
	    40, 39, 38, 37, 36, 35, 34, 33, 32, 31, 30, 29, 28, 27, 26, 25, 24, 23, 22, 21, 20, 19,    \
	    18, 17, 16, 15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0

// general definition for any function name
#define __CARYLL_VFUNC__2(name, n) name##n
#define __CARYLL_VFUNC__1(name, n) __CARYLL_VFUNC__2(name, n)
#define __CARYLLVFUNC(func, ...) __CARYLL_VFUNC__1(func, __NARG__(__VA_ARGS__))(__VA_ARGS__)

#define caryll_trivialInit(T)                                                                      \
	static __CARYLL_INLINE__ void T##_init(MODIFY T *x) {                                          \
		memset(x, 0, sizeof(T));                                                                   \
	}
#define caryll_nonTrivialInit(T, __fn_init)                                                        \
	static __CARYLL_INLINE__ void T##_init(MODIFY T *x) {                                          \
		(__fn_init)(x);                                                                            \
	}
#define caryll_trivialEmpty(T)                                                                     \
	static __CARYLL_INLINE__ T T##_empty() {                                                       \
		T x;                                                                                       \
		T##_init(&x);                                                                              \
		return x;                                                                                  \
	}
#define caryll_trivialCreate(T)                                                                    \
	static __CARYLL_INLINE__ T *T##_create() {                                                     \
		T *x = (T *)malloc(sizeof(T));                                                             \
		T##_init(x);                                                                               \
		return x;                                                                                  \
	}
#define caryll_trivialCopy(T)                                                                      \
	static __CARYLL_INLINE__ void T##_copy(MODIFY T *dst, const T *src) {                          \
		memcpy(dst, src, sizeof(T));                                                               \
	}
#define caryll_nonTrivialCopy(T, __fn_copy)                                                        \
	static __CARYLL_INLINE__ void T##_copy(MODIFY T *dst, const T *src) {                          \
		(__fn_copy)(dst, src);                                                                     \
	}
#define caryll_trivialMove(T)                                                                      \
	static __CARYLL_INLINE__ void T##_move(MODIFY T *dst, T *src) {                                \
		memcpy(dst, src, sizeof(T));                                                               \
		T##_init(src);                                                                             \
	}
#define caryll_nonTrivialMove(T, __fn_move)                                                        \
	static __CARYLL_INLINE__ void T##_move(MODIFY T *dst, T *src) {                                \
		(__fn_move)(dst, src);                                                                     \
		T##_init(src);                                                                             \
	}
#define caryll_trivialDispose(T)                                                                   \
	static __CARYLL_INLINE__ void T##_dispose(MODIFY T *x) {                                       \
		/* trivial */                                                                              \
	}
#define caryll_nonTrivialDispose(T, __fn_dispose)                                                  \
	static __CARYLL_INLINE__ void T##_dispose(MODIFY T *x) {                                       \
		(__fn_dispose)(x);                                                                         \
	}
#define caryll_trivialFree(T)                                                                      \
	static __CARYLL_INLINE__ void T##_free(MOVE T *x) {                                            \
		if (!x) return;                                                                            \
		T##_dispose(x);                                                                            \
		__caryll_free(x);                                                                          \
	}
#define caryll_trivialDup(T)                                                                       \
	static __CARYLL_INLINE__ T T##_dup(const T src) {                                              \
		T dst;                                                                                     \
		T##_copy(&dst, &src);                                                                      \
		return dst;                                                                                \
	}
#define caryll_trivialReplace(T)                                                                   \
	static __CARYLL_INLINE__ void T##_replace(MODIFY T *dst, MOVE const T src) {                   \
		T##_dispose(dst);                                                                          \
		memcpy(dst, &src, sizeof(T));                                                              \
	}                                                                                              \
	static __CARYLL_INLINE__ void T##_copyReplace(MODIFY T *dst, const T src) {                    \
		T##_dispose(dst);                                                                          \
		T##_copy(dst, &src);                                                                       \
	}

#define caryll_standardTypeFn1(T)                                                                  \
	caryll_trivialInit(T);                                                                         \
	caryll_trivialCopy(T);                                                                         \
	caryll_trivialDispose(T);                                                                      \
	caryll_trivialMove(T);                                                                         \
	caryll_trivialReplace(T);
#define caryll_standardTypeFn2(T, __fn_dispose)                                                    \
	caryll_trivialInit(T);                                                                         \
	caryll_trivialCopy(T);                                                                         \
	caryll_nonTrivialDispose(T, __fn_dispose);                                                     \
	caryll_trivialMove(T);                                                                         \
	caryll_trivialReplace(T);
#define caryll_standardTypeFn3(T, __fn_init, __fn_dispose)                                         \
	caryll_nonTrivialInit(T, __fn_init);                                                           \
	caryll_trivialCopy(T);                                                                         \
	caryll_nonTrivialDispose(T, __fn_dispose);                                                     \
	caryll_trivialMove(T);                                                                         \
	caryll_trivialReplace(T);
#define caryll_standardTypeFn4(T, __fn_init, __fn_copy, __fn_dispose)                              \
	caryll_nonTrivialInit(T, __fn_init);                                                           \
	caryll_nonTrivialCopy(T, __fn_copy);                                                           \
	caryll_nonTrivialDispose(T, __fn_dispose);                                                     \
	caryll_trivialMove(T);                                                                         \
	caryll_trivialReplace(T);
#define caryll_standardTypeFn5(T, __fn_init, __fn_copy, __fn_dispose, __fn_move)                   \
	caryll_nonTrivialInit(T, __fn_init);                                                           \
	caryll_nonTrivialCopy(T, __fn_copy);                                                           \
	caryll_nonTrivialDispose(T, __fn_dispose);                                                     \
	caryll_nonTrivialMove(T, __fn_move);                                                           \
	caryll_trivialReplace(T);

#define caryll_standardTypeMethods(T)                                                              \
	.init = T##_init, .copy = T##_copy, .dispose = T##_dispose, .move = T##_move,                  \
	.replace = T##_replace, .copyReplace = T##_copyReplace
#define caryll_standardRefTypeMethods(T)                                                           \
	caryll_standardTypeMethods(T), .create = T##_create, .free = T##_free
#define caryll_standardValTypeMethods(T)                                                           \
	caryll_standardTypeMethods(T), .empty = T##_empty, .dup = T##_dup

#define caryll_standardTypeFn(...) __CARYLLVFUNC(caryll_standardTypeFn, __VA_ARGS__)

#ifdef __GNUC__

// GCC requires adding ## before __VA_ARGS__.
// Make it happy.

#define caryll_standardRefTypeFn(T, ...)                                                           \
	caryll_standardTypeFn(T, ##__VA_ARGS__);                                                       \
	caryll_trivialCreate(T);                                                                       \
	caryll_trivialFree(T);
#define caryll_standardValTypeFn(T, ...)                                                           \
	caryll_standardTypeFn(T, ##__VA_ARGS__);                                                       \
	caryll_trivialEmpty(T);                                                                        \
	caryll_trivialDup(T);

#define caryll_standardType(T, __name, ...)                                                        \
	caryll_standardTypeFn(T, ##__VA_ARGS__);                                                       \
	caryll_ElementInterfaceOf(T) __name = {caryll_standardTypeMethods(T)};
#define caryll_standardRefType(T, __name, ...)                                                     \
	caryll_standardRefTypeFn(T, ##__VA_ARGS__);                                                    \
	caryll_ElementInterfaceOf(T) __name = {caryll_standardRefTypeMethods(T)};
#define caryll_standardValType(T, __name, ...)                                                     \
	caryll_standardValTypeFn(T, ##__VA_ARGS__);                                                    \
	caryll_ElementInterfaceOf(T) __name = {caryll_standardValTypeMethods(T)};

#else

#define caryll_standardRefTypeFn(T, ...)                                                           \
	caryll_standardTypeFn(T, __VA_ARGS__);                                                         \
	caryll_trivialCreate(T);                                                                       \
	caryll_trivialFree(T);
#define caryll_standardValTypeFn(T, ...)                                                           \
	caryll_standardTypeFn(T, __VA_ARGS__);                                                         \
	caryll_trivialEmpty(T);                                                                        \
	caryll_trivialDup(T);

#define caryll_standardType(T, __name, ...)                                                        \
	caryll_standardTypeFn(T, __VA_ARGS__);                                                         \
	caryll_ElementInterfaceOf(T) __name = {caryll_standardTypeMethods(T)};
#define caryll_standardRefType(T, __name, ...)                                                     \
	caryll_standardRefTypeFn(T, __VA_ARGS__);                                                      \
	caryll_ElementInterfaceOf(T) __name = {caryll_standardRefTypeMethods(T)};
#define caryll_standardValType(T, __name, ...)                                                     \
	caryll_standardValTypeFn(T, __VA_ARGS__);                                                      \
	caryll_ElementInterfaceOf(T) __name = {caryll_standardValTypeMethods(T)};

#endif

//// Traits

// Show
#define caryll_ShowFns(T, _fn_Show)                                                                \
	static __CARYLL_INLINE__ void T##_show(const T a) {                                            \
		return (_fn_Show)(a);                                                                      \
	}
#define caryll_ShowAssigns(T) .show = T##_show

// Ord
#define caryll_OrdFns(T, _fnCompare)                                                               \
	static __CARYLL_INLINE__ int T##_compare(const T a, const T b) {                               \
		return (_fnCompare)(a, b);                                                                 \
	}                                                                                              \
	static __CARYLL_INLINE__ int T##_compareRef(const T *a, const T *b) {                          \
		return (_fnCompare)(*a, *b);                                                               \
	}
#define caryll_OrdEqFns(T, _fnCompare)                                                             \
	caryll_OrdFns(T, _fnCompare);                                                                  \
	static __CARYLL_INLINE__ bool T##_equal(const T a, const T b) {                                \
		return !(_fnCompare)(a, b);                                                                \
	}
#define caryll_OrdEqAssigns(T)                                                                     \
	.compare = T##_compare, .compareRef = T##_compareRef, .equal = T##_equal

// Monoid
#define caryll_MonoidFns(T, _fn_neutral, _fn_inplacePlus)                                          \
	static __CARYLL_INLINE__ T T##_neutral() {                                                     \
		return (_fn_neutral)();                                                                    \
	}                                                                                              \
	static __CARYLL_INLINE__ void T##_inplacePlus(MODIFY T *a, const T b) {                        \
		(_fn_inplacePlus)(a, b);                                                                   \
	}                                                                                              \
	static __CARYLL_INLINE__ T T##_plus(const T a, const T b) {                                    \
		T result = (_fn_neutral)();                                                                \
		(_fn_inplacePlus)(&result, a);                                                             \
		(_fn_inplacePlus)(&result, b);                                                             \
		return result;                                                                             \
	}
#define caryll_MonoidAssigns(T)                                                                    \
	.neutral = T##_neutral, .inplacePlus = T##_inplacePlus, .plus = T##_plus

// Group
#define caryll_GroupFns(T, _fn_inplaceNegate)                                                      \
	static __CARYLL_INLINE__ void T##_inplaceNegate(MODIFY T *a) {                                 \
		(_fn_inplaceNegate)(a);                                                                    \
	}                                                                                              \
	static __CARYLL_INLINE__ T T##_negate(const T a) {                                             \
		T result;                                                                                  \
		T##_copy(&result, &a);                                                                     \
		T##_inplaceNegate(&result);                                                                \
		return result;                                                                             \
	}                                                                                              \
	static __CARYLL_INLINE__ void T##_inplaceMinus(MODIFY T *a, const T b) {                       \
		T tb = T##_negate(b);                                                                      \
		(T##_inplacePlus)(a, tb);                                                                  \
		T##_dispose(&tb);                                                                          \
	}                                                                                              \
	static __CARYLL_INLINE__ T T##_minus(const T a, const T b) {                                   \
		T result = (T##_neutral)();                                                                \
		(T##_inplacePlus)(&result, a);                                                             \
		(T##_inplaceMinus)(&result, b);                                                            \
		return result;                                                                             \
	}
#define caryll_GroupAssigns(T)                                                                     \
	.inplaceNegate = T##_inplaceNegate, .negate = T##_negate, .inplaceMinus = T##_inplaceMinus,    \
	.minus = T##_minus

// Module
#define caryll_ModuleFns(T, TS, _fn_inplaceScale)                                                  \
	static __CARYLL_INLINE__ void T##_inplaceScale(MODIFY T *a, TS b) {                            \
		(_fn_inplaceScale)(a, b);                                                                  \
	}                                                                                              \
	static __CARYLL_INLINE__ T T##_scale(const T a, TS b) {                                        \
		T result;                                                                                  \
		T##_copy(&result, &a);                                                                     \
		T##_inplaceScale(&result, b);                                                              \
		return result;                                                                             \
	}                                                                                              \
	static __CARYLL_INLINE__ void T##_inplacePlusScale(MODIFY T *a, TS b, const T c) {             \
		T x = T##_scale(c, b);                                                                     \
		T##_inplacePlus(a, x);                                                                     \
		T##_dispose(&x);                                                                           \
	}
#define caryll_ModuleAssigns(T)                                                                    \
	.inplaceScale = T##_inplaceScale, .scale = T##_scale, .inplacePlusScale = T##_inplacePlusScale

#endif
