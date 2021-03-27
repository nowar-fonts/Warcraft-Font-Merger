#ifndef CARYLL_SUPPORT_ALIASES_H
#define CARYLL_SUPPORT_ALIASES_H

#include <stdint.h>

#define loggedStep(...)                                                                            \
	for (bool ___loggedstep_v = (options->logger->startSDS(options->logger,                        \
	                                                       sdscatprintf(sdsempty(), __VA_ARGS__)), \
	                             true);                                                            \
	     ___loggedstep_v; ___loggedstep_v = false, options->logger->finish(options->logger))
#define logError(...)                                                                              \
	options->logger->logSDS(options->logger, log_vl_critical, log_type_error,                      \
	                        sdscatprintf(sdsempty(), __VA_ARGS__));
#define logWarning(...)                                                                            \
	options->logger->logSDS(options->logger, log_vl_important, log_type_warning,                   \
	                        sdscatprintf(sdsempty(), __VA_ARGS__));
#define logNotice(...)                                                                             \
	options->logger->logSDS(options->logger, log_vl_notice, log_type_info,                         \
	                        sdscatprintf(sdsempty(), __VA_ARGS__));
#define logProgress(...)                                                                           \
	options->logger->logSDS(options->logger, log_vl_progress, log_type_progress,                   \
	                        sdscatprintf(sdsempty(), __VA_ARGS__));

#define FOR_TABLE_SILENT(name, table)                                                              \
	for (int __fortable_keep = 1, __fortable_count = 0, __notfound = 1;                            \
	     __notfound && __fortable_keep && __fortable_count < packet.numTables;                     \
	     __fortable_keep = !__fortable_keep, __fortable_count++)                                   \
		for (otfcc_PacketPiece table = (packet.pieces)[__fortable_count]; __fortable_keep;         \
		     __fortable_keep = !__fortable_keep)                                                   \
			if (table.tag == (name))                                                               \
				for (int __fortable_k2 = 1; __fortable_k2; __fortable_k2 = 0, __notfound = 0)

#define FOR_TABLE(name, table) FOR_TABLE_SILENT(name, table)

#define foreach_index(item, index, vector)                                                         \
	for (size_t index = 0, keep = 1; keep && index < (vector).length; keep = !keep, index++)       \
		for (item = (vector).items + index; keep; keep = !keep)
#define foreach(item, vector) foreach_index(item, __caryll_index, vector)
#define foreach_hash(id, range) for (id = (range); id != NULL; id = id->hh.next)

typedef uint8_t *font_file_pointer;

// alias "package" otfcc_pHandle to Handle
#include "otfcc/handle.h"
#define Handle otfcc_iHandle
typedef otfcc_GlyphHandle glyph_handle;
typedef otfcc_FDHandle fd_handle;
typedef otfcc_LookupHandle lookup_handle;

// alias "package" otfcc_pkgGlyphOrder to GlyphOrder
#include "otfcc/glyph-order.h"
#define GlyphOrder otfcc_pkgGlyphOrder

#include "otfcc/table/otl/classdef.h"
#define ClassDef otl_iClassDef

#include "otfcc/table/otl/coverage.h"
#define Coverage otl_iCoverage

#endif
