/*
 * tests_qformat.c - Unit tests for bounded formatting / string-copy helpers
 * in qcommon/q_shared.c: Com_sprintf, va, Q_strncpy.
 *
 * Com_sprintf is the length-bounded sprintf used throughout the engine; we
 * verify it respects the size limit (NUL-terminates, never overruns) and
 * returns the expected length. Q_strncpy is the overlap-safe copy.
 *
 * Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c tests_qmath.c tests_qinfo.c \
 *       tests_qstr.c tests_qmatrix.c tests_qtoken.c tests_qchar.c \
 *       tests_qhuffman.c tests_qmsg.c tests_qformat.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/huffman.c ../code/qcommon/msg.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"

TEST(com_sprintf_basic)
{
	char buf[64];
	int len = Com_sprintf(buf, sizeof(buf), "%s = %d", "score", 42);
	OA_ASSERT_STR(buf, "score = 42");
	OA_ASSERT_INT(len, 10);
}

TEST(com_sprintf_multiple_args)
{
	char buf[64];
	Com_sprintf(buf, sizeof(buf), "%d-%d-%d", 1, 2, 3);
	OA_ASSERT_STR(buf, "1-2-3");
}

TEST(com_sprintf_respects_size_limit)
{
	/* dest is only 8 bytes; the output must be truncated and NUL-terminated
	 * at index 7, never writing past buf[7]. A canary after the buffer
	 * proves no overrun. */
	char buf[8];
	char canary[4];
	memset(canary, 0xAA, sizeof(canary));
	int len = Com_sprintf(buf, sizeof(buf), "0123456789");
	/* string is cut to 7 chars + NUL */
	OA_ASSERT_STRN(buf, "0123456", 8);
	OA_ASSERT_INT(buf[7], 0);
	/* canary untouched => no write past the buffer (compare as unsigned
	 * char; 0xAA as signed char is -86) */
	OA_ASSERT_INT((unsigned char)canary[0], 0xAA);
	OA_ASSERT_INT((unsigned char)canary[1], 0xAA);
	/* Com_sprintf returns the would-be length on truncation */
	OA_ASSERT_INT(len, 10);
}

TEST(com_sprintf_float)
{
	char buf[64];
	Com_sprintf(buf, sizeof(buf), "%.2f", 3.14159f);
	OA_ASSERT_STR(buf, "3.14");
}

TEST(va_basic)
{
	/* va() returns a pointer to a static buffer holding the formatted text */
	char *s = va("%s:%d", "port", 27960);
	OA_ASSERT_STR(s, "port:27960");
}

TEST(q_strncpy_basic)
{
	char dest[16];
	Q_strncpy(dest, "hello", sizeof(dest));
	OA_ASSERT_STR(dest, "hello");
}

TEST(q_strncpy_pads_with_zero)
{
	/* Q_strncpy zero-fills the remainder of the destination buffer */
	char dest[8];
	memset(dest, 0xFF, sizeof(dest));
	Q_strncpy(dest, "ab", sizeof(dest));
	OA_ASSERT_INT(dest[0], 'a');
	OA_ASSERT_INT(dest[1], 'b');
	OA_ASSERT_INT(dest[2], 0);
	OA_ASSERT_INT(dest[3], 0);
}

TEST(q_strncpy_truncates_at_count)
{
	char dest[8];
	Q_strncpy(dest, "toolong", 4); /* count limits copy to 4 chars, no NUL */
	OA_ASSERT_INT(dest[0], 't');
	OA_ASSERT_INT(dest[1], 'o');
	OA_ASSERT_INT(dest[2], 'o');
	OA_ASSERT_INT(dest[3], 'l');
	/* dest[4] is untouched by Q_strncpy (no NUL appended) */
}
