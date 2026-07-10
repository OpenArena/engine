/*
 * tests_qinfo_overflow.c - Overflow-safety tests for the info-string setters.
 *
 * These deliberately feed an oversize / boundary input to Info_SetValueForKey
 * and verify that the engine does NOT write past the end of the caller's
 * buffer (a classic Q3 buffer-overflow class). To assert this we use a
 * non-aborting Com_Error stub (a buffer overrun must not be fatal-to-test)
 * and place a canary word right after the info buffer.
 *
 * This file is compiled into a SEPARATE binary (test_overflow) with its own
 * stub, so the non-aborting Com_Error does not affect the other suites.
 *
 * Build & run:
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_overflow \
 *       oa_test_run.c test_main.c test_stubs_nofail.c tests_qinfo_overflow.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_overflow
 */
#include "oa_test.h"

#include "q_shared.h"

/* This suite needs a Com_Error that records-but-does-not-abort, so it is built
 * with test_stubs_nofail.c instead of test_stubs.c. We just use the harness
 * assertions; no extra machinery needed here. */

TEST(info_set_value_does_not_overflow_on_long_value)
{
	/* Fill a MAX_INFO_STRING buffer with a valid pre-existing key, then try
	 * to append a value that would exceed the limit. The setter must refuse
	 * (print "Info string length exceeded") and must NOT write past the
	 * buffer. We detect overflow with a canary placed immediately after. */
	char buf[2 * MAX_INFO_STRING];
	char *s = buf;                 /* info string lives in the first half */
	/* lay a known canary pattern right after the info buffer region */
	unsigned char *canary = (unsigned char *)(buf + MAX_INFO_STRING);
	for (int i = 0; i < MAX_INFO_STRING; i++)
		canary[i] = 0xA5;

	/* start with one small key so the string is non-empty */
	strcpy(s, "\\name\\x");

	/* build a value that is way too long (> MAX_INFO_STRING) */
	char bigval[MAX_INFO_STRING * 2];
	memset(bigval, 'a', sizeof(bigval) - 1);
	bigval[sizeof(bigval) - 1] = 0;

	Info_SetValueForKey(s, "big", bigval);

	/* the canary region must be untouched: no write past MAX_INFO_STRING */
	int corrupted = 0;
	for (int i = 0; i < MAX_INFO_STRING; i++) {
		if (canary[i] != 0xA5) {
			corrupted = 1;
			break;
		}
	}
	OA_ASSERT_INT(corrupted, 0);

	/* and the original small key must still be intact/parseable */
	OA_ASSERT_STR(Info_ValueForKey(s, "name"), "x");
}
