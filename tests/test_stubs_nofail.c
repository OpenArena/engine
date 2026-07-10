/*
 * test_stubs_nofail.c - Like test_stubs.c but Com_Error records the error
 * instead of aborting. Used by the overflow-safety tests, where triggering
 * the engine's oversize guard must NOT kill the test process (a buffer
 * overrun guard firing is the *expected* behaviour we want to observe).
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <assert.h>

#include "q_shared.h"

int oa_com_error_code = 0; /* last Com_Error code, observable by tests */

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
	fprintf(stderr, "Com_Error(nonfatal,%d): ", code);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	oa_com_error_code = code;
	/* NOTE: do NOT abort() here so the test can continue and check the
	 * buffer afterwards. */
}

int QDECL FS_ReadFile(const char *qpath, void **buffer)
{
	(void)qpath; (void)buffer;
	return -1;
}

void QDECL FS_FreeFile(void *buffer)
{
	(void)buffer;
}
