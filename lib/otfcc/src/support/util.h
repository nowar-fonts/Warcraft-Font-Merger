#ifndef CARYLL_SUPPORT_UTIL_H
#define CARYLL_SUPPORT_UTIL_H

#ifndef __STDC_FORMAT_MACROS
#define __STDC_FORMAT_MACROS
#endif

#include <inttypes.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "dep/json-builder.h"
#include "dep/json.h"

#include "caryll/ownership.h"
#include "caryll/buffer.hpp"

#include "otfcc/handle.hpp"
#include "otfcc/primitives.hpp"
#include "otfcc/options.hpp"

#include "aliases.h"
#include "otfcc-alloc.h"
#include "element-impl.h"
#include "vector-impl.h"

#include "base64/base64.h"
#include "json/json-funcs.hpp"
#include "bin-io.h"
#include "tag.h"

#endif
