/*
 * tests_qstring2.c - More unit tests for qcommon/q_shared.c string helpers
 * not yet covered: Q_strlwr, Q_strupr, Q_strcat (bounded append), Q_CountChar.
 *
 * All SDL/OpenGL-free. Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I.. -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c tests_qmath.c tests_qinfo.c \
 *       tests_qstr.c tests_qmatrix.c tests_qtoken.c tests_qchar.c \
 *       tests_qhuffman.c tests_qmsg.c tests_qformat.c tests_qmath2.c \
 *       tests_qmath3.c tests_qstring2.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/huffman.c ../code/qcommon/msg.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"

TEST(q_strlwr_basic)
{
	char s[] = "HeLLo WoRLD";
	Q_strlwr(s);
	OA_ASSERT_STR(s, "hello world");
}

TEST(q_strlwr_non_alpha_untouched)
{
	char s[] = "A1B2C3!@#";
	Q_strlwr(s);
	OA_ASSERT_STR(s, "a1b2c3!@#");
}

TEST(q_strupr_basic)
{
	char s[] = "HeLLo WoRLD";
	Q_strupr(s);
	OA_ASSERT_STR(s, "HELLO WORLD");
}

TEST(q_strcat_basic)
{
	char dest[32] = "hello";
	Q_strcat(dest, sizeof(dest), " world");
	OA_ASSERT_STR(dest, "hello world");
}

TEST(q_strcat_into_empty)
{
	char dest[32] = "";
	Q_strcat(dest, sizeof(dest), "abc");
	OA_ASSERT_STR(dest, "abc");
}

TEST(q_strcat_respects_bound)
{
	/* dest has only 8 bytes; appending must be truncated to fit and stay
	 * NUL-terminated (Q_strcat uses Q_strncpyz with size - existing_len).
	 * A canary after the buffer proves no overrun. */
	char dest[8];
	char canary[4];
	memset(canary, 0xAA, sizeof(canary));
	dest[0] = 'a';
	dest[1] = 'b';   /* existing length 2 */
	dest[2] = 0;
	Q_strcat(dest, sizeof(dest), "cdefghij"); /* only 5 more bytes fit (idx 2..6 + NUL) */
	OA_ASSERT_STRN(dest, "abcdefg", 8);
	OA_ASSERT_INT(dest[7], 0);
	OA_ASSERT_INT((unsigned char)canary[0], 0xAA);
}

TEST(q_count_char_present)
{
	OA_ASSERT_INT(Q_CountChar("a.b.c.d", '.'), 3);
}

TEST(q_count_char_absent)
{
	OA_ASSERT_INT(Q_CountChar("abcdef", 'z'), 0);
}

TEST(q_count_char_empty)
{
	OA_ASSERT_INT(Q_CountChar("", 'x'), 0);
}

TEST(q_count_char_repeated)
{
	OA_ASSERT_INT(Q_CountChar("aaaaa", 'a'), 5);
}
