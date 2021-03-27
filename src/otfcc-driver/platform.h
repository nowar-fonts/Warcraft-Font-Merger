#ifndef CARYLL_CLI_PLATFORM_H
#define CARYLL_CLI_PLATFORM_H

#ifdef _WIN32

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <windows.h>
#include "getopt.h"

#ifdef _MSC_VER
#include "winfns.h"
#endif

int get_argv_utf8(int *argc_ptr, char ***argv_ptr) {
	int argc;
	char **argv;
	wchar_t **argv_utf16 = CommandLineToArgvW(GetCommandLineW(), &argc);
	int i;
	int offset = (argc + 1) * sizeof(char *);
	int size = offset;
	for (i = 0; i < argc; i++) {
		size += WideCharToMultiByte(CP_UTF8, 0, argv_utf16[i], -1, 0, 0, 0, 0);
	}
	argv = (char **)malloc(size);
	for (i = 0; i < argc; i++) {
		argv[i] = (char *)argv + offset;
		offset += WideCharToMultiByte(CP_UTF8, 0, argv_utf16[i], -1, argv[i], size - offset, 0, 0);
	}
	*argc_ptr = argc;
	*argv_ptr = argv;
	return 0;
}
int widen_utf8(char *filename_utf8, LPWSTR *filename_w) {
	int num_chars = MultiByteToWideChar(CP_UTF8, 0, filename_utf8, -1, 0, 0);
	int size = sizeof(WCHAR);
	*filename_w = (LPWSTR)malloc(size * num_chars);
	MultiByteToWideChar(CP_UTF8, 0, filename_utf8, -1, *filename_w, num_chars);
	return num_chars;
}
FILE *__u8fopen(char *path, char *mode) {
	LPWSTR wpath, wmode;
	widen_utf8(path, &wpath);
	widen_utf8(mode, &wmode);
	FILE *f = _wfopen(wpath, wmode);
	free(wpath);
	free(wmode);
	return f;
}
#define u8fopen __u8fopen

#ifdef _MSC_VER
#include <io.h>
#define isatty _isatty
#define fileno _fileno
#elif __MINGW32__
#include <unistd.h>
#endif

#else

#include <getopt.h>
#define u8fopen fopen

#endif
#endif
