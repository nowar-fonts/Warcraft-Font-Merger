#include "stopwatch.h"
#ifdef _WIN32
LARGE_INTEGER getFILETIMEoffset() {
	SYSTEMTIME s;
	FILETIME f;
	LARGE_INTEGER t;

	s.wYear = 1970;
	s.wMonth = 1;
	s.wDay = 1;
	s.wHour = 0;
	s.wMinute = 0;
	s.wSecond = 0;
	s.wMilliseconds = 0;
	SystemTimeToFileTime(&s, &f);
	t.QuadPart = f.dwHighDateTime;
	t.QuadPart <<= 32;
	t.QuadPart |= f.dwLowDateTime;
	return (t);
}

void time_now(struct timespec *tv) {
	LARGE_INTEGER t;
	FILETIME f;
	double microseconds;
	static LARGE_INTEGER offset;
	static double frequencyToMicroseconds;
	static int initialized = 0;
	static BOOL usePerformanceCounter = 0;

	if (!initialized) {
		LARGE_INTEGER performanceFrequency;
		initialized = 1;
		usePerformanceCounter = QueryPerformanceFrequency(&performanceFrequency);
		if (usePerformanceCounter) {
			QueryPerformanceCounter(&offset);
			frequencyToMicroseconds = (double)performanceFrequency.QuadPart / 1000000.0;
		} else {
			offset = getFILETIMEoffset();
			frequencyToMicroseconds = 10.0;
		}
	}
	if (usePerformanceCounter)
		QueryPerformanceCounter(&t);
	else {
		GetSystemTimeAsFileTime(&f);
		t.QuadPart = f.dwHighDateTime;
		t.QuadPart <<= 32;
		t.QuadPart |= f.dwLowDateTime;
	}

	t.QuadPart -= offset.QuadPart;
	microseconds = (double)t.QuadPart / frequencyToMicroseconds;
	t.QuadPart = microseconds;
	tv->tv_sec = t.QuadPart / 1000000;
	tv->tv_nsec = t.QuadPart % 1000000 * 1000;
}
#elif __MACH__
#define ORWL_NANO (+1.0E-9)
#define ORWL_GIGA UINT64_C(1000000000)

static double orwl_timebase = 0.0;
static uint64_t orwl_timestart = 0;

void time_now(struct timespec *tv) {
	// be more careful in a multithreaded environement
	if (!orwl_timestart) {
		mach_timebase_info_data_t tb = {0};
		mach_timebase_info(&tb);
		orwl_timebase = tb.numer;
		orwl_timebase /= tb.denom;
		orwl_timestart = mach_absolute_time();
	}
	double diff = (mach_absolute_time() - orwl_timestart) * orwl_timebase;
	tv->tv_sec = diff * ORWL_NANO;
	tv->tv_nsec = diff - (tv->tv_sec * ORWL_GIGA);
}
#else
void time_now(struct timespec *tv) {
	clock_gettime(CLOCK_REALTIME, tv);
}
#endif

#define BILLION 1000000000

static void timespec_diff(struct timespec *start, struct timespec *stop, struct timespec *result) {
	if ((stop->tv_nsec - start->tv_nsec) < 0) {
		result->tv_sec = stop->tv_sec - start->tv_sec - 1;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec + BILLION;
	} else {
		result->tv_sec = stop->tv_sec - start->tv_sec;
		result->tv_nsec = stop->tv_nsec - start->tv_nsec;
	}

	return;
}

sds push_stopwatch(struct timespec *sofar) {
	struct timespec ends;
	time_now(&ends);
	struct timespec diff;
	timespec_diff(sofar, &ends, &diff);
	*sofar = ends;
	sds log = sdscatprintf(sdsempty(), "Step time = %gs.\n", diff.tv_sec + diff.tv_nsec / (double)BILLION);
	return log;
}
