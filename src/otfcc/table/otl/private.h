#ifndef CARYLL_TABLE_OTL_PRIVATE_H
#define CARYLL_TABLE_OTL_PRIVATE_H

#include "subtables/gsub-single.h"
#include "subtables/gsub-multi.h"
#include "subtables/gsub-ligature.h"
#include "subtables/gsub-reverse.h"
#include "subtables/gpos-single.h"
#include "subtables/gpos-pair.h"
#include "subtables/gpos-cursive.h"
#include "subtables/gpos-mark-to-single.h"
#include "subtables/gpos-mark-to-ligature.h"
#include "subtables/chaining.h"
#include "subtables/extend.h"

extern const char SCRIPT_LANGUAGE_SEPARATOR;
extern const char *lookupFlagsLabels[];
extern const char *tableNames[];

void otfcc_delete_lookup(otl_Lookup *lookup);

#endif
