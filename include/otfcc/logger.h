#ifndef CARYLL_INCLUDE_LOGGER_H
#define CARYLL_INCLUDE_LOGGER_H

#include "dep/sds.h"
#include "caryll/ownership.h"
#include "primitives.h"

typedef struct otfcc_ILoggerTarget {
	void (*dispose)(struct otfcc_ILoggerTarget *self);             // destructor
	void (*push)(struct otfcc_ILoggerTarget *self, MOVE sds data); // push data
} otfcc_ILoggerTarget;

typedef enum { log_type_error = 0, log_type_warning = 1, log_type_info = 2, log_type_progress = 3 } otfcc_LoggerType;
enum { log_vl_critical = 0, log_vl_important = 1, log_vl_notice = 2, log_vl_info = 5, log_vl_progress = 10 };

typedef struct otfcc_ILogger {
	void (*dispose)(struct otfcc_ILogger *self);                     // destructor
	void (*indent)(struct otfcc_ILogger *self, const char *segment); // add a level
	void (*indentSDS)(struct otfcc_ILogger *self, MOVE sds segment); // add a level, using SDS
	void (*start)(struct otfcc_ILogger *self, const char *segment);  // add a level, output a progress
	void (*startSDS)(struct otfcc_ILogger *self, MOVE sds segment);  // add a level, output a progress
	void (*log)(struct otfcc_ILogger *self, uint8_t verbosity, otfcc_LoggerType type, const char *data); // log a data
	void (*logSDS)(struct otfcc_ILogger *self, uint8_t verbosity, otfcc_LoggerType type, MOVE sds data); // log a data
	void (*dedent)(struct otfcc_ILogger *self);                          // remove a level
	void (*finish)(struct otfcc_ILogger *self);                          // remove a level, finishing a ask
	void (*end)(struct otfcc_ILogger *self);                             // remove a level
	void (*setVerbosity)(struct otfcc_ILogger *self, uint8_t verbosity); // remove a level
	otfcc_ILoggerTarget *(*getTarget)(struct otfcc_ILogger *self);       // query target
} otfcc_ILogger;

otfcc_ILogger *otfcc_newLogger(otfcc_ILoggerTarget *target);
otfcc_ILoggerTarget *otfcc_newStdErrTarget();
otfcc_ILoggerTarget *otfcc_newEmptyTarget();

#endif
