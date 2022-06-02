#ifndef CARYLL_SUPPORT_STOPWATCH_H
#define CARYLL_SUPPORT_STOPWATCH_H

#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "dep/sds.h"

#ifdef _WIN32
// Windows
#include <windows.h>
#elif __MACH__
// OSX
#include <unistd.h>
#include <mach/mach_time.h>
#else
#include <unistd.h>
#endif
void time_now(struct timespec *tv);
sds push_stopwatch(struct timespec *sofar);
#define logStepTime                                                                                                    \
	options->logger->logSDS(options->logger, log_vl_progress, log_type_progress, push_stopwatch(&begin));

#endif
