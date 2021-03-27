#include "support/util.h"
#include "otfcc/logger.h"

typedef struct Logger {
	otfcc_ILogger vtable;
	void *target;
	uint16_t level;
	uint16_t lastLoggedLevel;
	uint16_t levelCap;
	sds *indents;
	uint8_t verbosityLimit;
} Logger;

const char *otfcc_LoggerTypeNames[3] = {"[ERROR]", "[WARNING]", "[NOTE]"};

static void loggerIndent(otfcc_ILogger *_self, const char *segment) {
	((otfcc_ILogger *)_self)->indentSDS(_self, sdsnew(segment));
}
static void loggerIndentSDS(otfcc_ILogger *_self, MOVE sds segment) {
	Logger *self = (Logger *)_self;
	uint8_t newLevel = self->level + 1;
	if (newLevel > self->levelCap) {
		self->levelCap += self->levelCap / 2 + 1;
		RESIZE(self->indents, self->levelCap);
	}
	self->level++;
	self->indents[self->level - 1] = segment;
}

static void loggerDedent(otfcc_ILogger *_self) {
	Logger *self = (Logger *)_self;
	if (!self->level) return;
	sdsfree(self->indents[self->level - 1]);
	self->level -= 1;
	if (self->level < self->lastLoggedLevel) { self->lastLoggedLevel = self->level; }
}
static void loggerFinish(otfcc_ILogger *self) {
	self->logSDS(self, log_vl_progress + ((Logger *)self)->level, log_type_progress, sdsnew("Finish"));
	self->dedent(self);
}
static void loggerStart(otfcc_ILogger *self, const char *segment) {
	self->indentSDS(self, sdsnew(segment));
	self->logSDS(self, log_vl_progress + ((Logger *)self)->level, log_type_progress, sdsnew("Begin"));
}
static void loggerStartSDS(otfcc_ILogger *self, MOVE sds segment) {
	self->indentSDS(self, segment);
	self->logSDS(self, log_vl_progress + ((Logger *)self)->level, log_type_progress, sdsnew("Begin"));
}
static void loggerLog(otfcc_ILogger *self, uint8_t verbosity, otfcc_LoggerType type, const char *data) {
	self->logSDS(self, verbosity, type, sdsnew(data));
}

static void loggerLogSDS(otfcc_ILogger *_self, uint8_t verbosity, otfcc_LoggerType type, MOVE sds data) {
	Logger *self = (Logger *)_self;
	sds demand = sdsempty();
	for (uint16_t level = 0; level < self->level; level++) {
		if (level < self->lastLoggedLevel - 1) {
			for (size_t j = 0; j < sdslen(self->indents[level]); j++) {
				demand = sdscat(demand, " ");
			}
			if (level < self->lastLoggedLevel - 2) {
				demand = sdscat(demand, " | ");
			} else {
				demand = sdscat(demand, " |-");
			}
		} else {
			demand = sdscatfmt(demand, "%S : ", self->indents[level]);
		}
	}
	if (type < 3) {
		demand = sdscatfmt(demand, "%s %S", otfcc_LoggerTypeNames[type], data);
	} else {
		demand = sdscatfmt(demand, "%S", data);
	}
	sdsfree(data);
	if (verbosity <= self->verbosityLimit) {
		_self->getTarget(_self)->push(self->target, demand);
		self->lastLoggedLevel = self->level;
	} else {
		sdsfree(demand);
	}
}

static otfcc_ILoggerTarget *loggerGetTarget(otfcc_ILogger *_self) {
	Logger *self = (Logger *)_self;
	return (otfcc_ILoggerTarget *)self->target;
}

static void loggerSetVerbosity(otfcc_ILogger *_self, uint8_t verbosity) {
	Logger *self = (Logger *)_self;
	self->verbosityLimit = verbosity;
}

static INLINE void loggerDispose(otfcc_ILogger *_self) {
	Logger *self = (Logger *)_self;
	if (!self) return;
	otfcc_ILoggerTarget *target = _self->getTarget(_self);
	target->dispose(target);
	for (uint16_t level = 0; level < self->level; level++) {
		sdsfree(self->indents[level]);
	}
	FREE(self->indents);
	FREE(self);
}

const otfcc_ILogger VTABLE_LOGGER = {.dispose = loggerDispose,
                                     .indent = loggerIndent,
                                     .indentSDS = loggerIndentSDS,
                                     .start = loggerStart,
                                     .startSDS = loggerStartSDS,
                                     .log = loggerLog,
                                     .logSDS = loggerLogSDS,
                                     .dedent = loggerDedent,
                                     .finish = loggerFinish,
                                     .getTarget = loggerGetTarget,
                                     .setVerbosity = loggerSetVerbosity};

otfcc_ILogger *otfcc_newLogger(otfcc_ILoggerTarget *target) {
	Logger *logger;
	NEW(logger);
	logger->target = target;
	logger->vtable = VTABLE_LOGGER;
	return (otfcc_ILogger *)logger;
}

// STDERR logger target

typedef struct StderrTarget { otfcc_ILoggerTarget vtable; } StderrTarget;

void stderrTargetDispose(otfcc_ILoggerTarget *_self) {
	StderrTarget *self = (StderrTarget *)_self;
	if (!self) return;
	FREE(self);
}

void stderrTargetPush(otfcc_ILoggerTarget *_self, MOVE sds data) {
	fprintf(stderr, "%s", data);
	if (data[sdslen(data) - 1] != '\n') fprintf(stderr, "\n");
	sdsfree(data);
}

const otfcc_ILoggerTarget VTABLE_STDERR_TARGET = {.dispose = stderrTargetDispose, .push = stderrTargetPush};

otfcc_ILoggerTarget *otfcc_newStdErrTarget() {
	StderrTarget *target;
	NEW(target);
	target->vtable = VTABLE_STDERR_TARGET;
	return (otfcc_ILoggerTarget *)target;
}

// Empty logger target

typedef struct EmptyTarget { otfcc_ILoggerTarget vtable; } EmptyTarget;
void emptyTargetDispose(otfcc_ILoggerTarget *_self) {
	StderrTarget *self = (StderrTarget *)_self;
	if (!self) return;
	FREE(self);
}

void emptyTargetPush(otfcc_ILoggerTarget *_self, MOVE sds data) {
	sdsfree(data);
}

const otfcc_ILoggerTarget VTABLE_EMPTY_TARGET = {.dispose = emptyTargetDispose, .push = emptyTargetPush};

otfcc_ILoggerTarget *otfcc_newEmptyTarget() {
	StderrTarget *target;
	NEW(target);
	target->vtable = VTABLE_EMPTY_TARGET;
	return (otfcc_ILoggerTarget *)target;
}
