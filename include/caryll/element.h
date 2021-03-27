#ifndef CARYLL_INCLUDE_ELEMENT_H
#define CARYLL_INCLUDE_ELEMENT_H

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "ownership.h"

// We assume all T have trivial move constructors.
#define caryll_T(T)                                                                                \
	void (*init)(MODIFY T *);                                                                      \
	void (*copy)(MODIFY T *, const T *);                                                           \
	void (*move)(MODIFY T *, T *);                                                                 \
	void (*dispose)(MOVE T *);                                                                     \
	void (*replace)(MODIFY T *, MOVE const T);                                                     \
	void (*copyReplace)(MODIFY T *, const T);

#define caryll_VT(T)                                                                               \
	caryll_T(T);                                                                                   \
	T (*empty)();                                                                                  \
	T (*dup)(const T);
#define caryll_RT(T)                                                                               \
	caryll_T(T);                                                                                   \
	T *(*create)();                                                                                \
	void (*free)(MOVE T *);

#define caryll_ElementInterfaceOf(T) const struct __caryll_elementinterface_##T
#define caryll_ElementInterface(T)                                                                 \
	caryll_ElementInterfaceOf(T) {                                                                 \
		caryll_T(T);                                                                               \
	}
#define caryll_RefElementInterface(T)                                                              \
	caryll_ElementInterfaceOf(T) {                                                                 \
		caryll_RT(T);                                                                              \
	}
#define caryll_ValElementInterface(T)                                                              \
	caryll_ElementInterfaceOf(T) {                                                                 \
		caryll_VT(T);                                                                              \
	}

/// Individual traits

#define caryll_Show(T) void (*show)(const T);
#define caryll_Eq(T) bool (*equal)(const T, const T);
#define caryll_Ord(T)                                                                              \
	caryll_Eq(T);                                                                                  \
	int (*compare)(const T a, const T b);                                                          \
	int (*compareRef)(const T *a, const T *b);
#define caryll_Monoid(T)                                                                           \
	T (*neutral)();                                                                                \
	T (*plus)(const T a, const T b);                                                               \
	void (*inplacePlus)(MODIFY T * a, const T b);
#define caryll_Group(T)                                                                            \
	caryll_Monoid(T);                                                                              \
	void (*inplaceNegate)(MODIFY T * a);                                                           \
	T (*negate)(const T);                                                                          \
	void (*inplaceMinus)(MODIFY T *, const T);                                                     \
	T (*minus)(const T, const T);
#define caryll_Module(T, TScale)                                                                   \
	caryll_Group(T);                                                                               \
	void (*inplaceScale)(MODIFY T * a, TScale b);                                                  \
	void (*inplacePlusScale)(MODIFY T * a, TScale b, const T c);                                   \
	T (*scale)(const T a, TScale b);

#endif
