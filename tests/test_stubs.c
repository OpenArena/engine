/*
 * test_stubs.c - Minimal stubs for engine symbols referenced by q_shared.c
 * so the string/parse helpers can be unit-tested standalone (no full engine).
 *
 * Com_Error is fatal in the engine; our tests avoid the fatal inputs (NULL
 * dest/src), so this abort should never trigger during a passing run.
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <assert.h>

#include "q_shared.h"

void QDECL Com_Printf(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	vfprintf(stdout, fmt, ap);
	va_end(ap);
}

void QDECL Com_DPrintf(const char *fmt, ...)
{
	(void)fmt;
}

void QDECL Com_Error(int code, const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	fprintf(stderr, "Com_Error(%d): ", code);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	abort();
}

/* random other symbols occasionally pulled in by q_shared.c */
int QDECL FS_ReadFile(const char *qpath, void **buffer)
{
	(void)qpath; (void)buffer;
	return -1;
}

void QDECL FS_FreeFile(void *buffer)
{
	(void)buffer;
}
