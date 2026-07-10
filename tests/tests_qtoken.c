/*
 * tests_qtoken.c - More unit tests for qcommon/q_shared.c token/skip helpers
 * not already covered: Com_SkipCharset, Com_SkipTokens, and additional
 * COM_Parse edge cases (quoted tokens containing spaces, multiple tokens,
 * empty/whitespace-only input).
 *
 * All SDL/OpenGL-free. Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c tests_qmath.c tests_qinfo.c \
 *       tests_qstr.c tests_qmatrix.c tests_qtoken.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"

TEST(com_skip_charset_none)
{
	char s[] = "abc";
	OA_ASSERT_STR(Com_SkipCharset(s, " \t"), "abc");
}

TEST(com_skip_charset_spaces)
{
	char s[] = "   abc";
	OA_ASSERT_STR(Com_SkipCharset(s, " "), "abc");
}

TEST(com_skip_charset_mixed_sep)
{
	char s[] = "\t\n\r hello";
	OA_ASSERT_STR(Com_SkipCharset(s, " \t\n\r"), "hello");
}

TEST(com_skip_tokens_zero)
{
	char s[] = "a b c";
	OA_ASSERT_STR(Com_SkipTokens(s, 0, " "), "a b c");
}

TEST(com_skip_tokens_one)
{
	char s[] = "a b c";
	OA_ASSERT_STR(Com_SkipTokens(s, 1, " "), "b c");
}

TEST(com_skip_tokens_all)
{
	/* "a b c" has only TWO separators (spaces), not three. Com_SkipTokens
	 * counts separators, so requesting 2 lands past "b" on "c"; requesting
	 * 3 (more than present) returns the original string. */
	char s[] = "a b c";
	OA_ASSERT_STR(Com_SkipTokens(s, 2, " "), "c");
	OA_ASSERT_STR(Com_SkipTokens(s, 3, " "), "a b c");
}

TEST(com_skip_tokens_fewer_than_requested)
{
	/* requesting more separators than exist returns the original string */
	char s[] = "a b c";
	OA_ASSERT_STR(Com_SkipTokens(s, 9, " "), "a b c");
}

TEST(com_parse_quoted_with_space)
{
	char buf[] = "\"hello world\" next";
	char *p = buf;
	OA_ASSERT_STR(COM_Parse(&p), "hello world");
	OA_ASSERT_STR(COM_Parse(&p), "next");
}

TEST(com_parse_multiple_tokens)
{
	char buf[] = "one two three four";
	char *p = buf;
	OA_ASSERT_STR(COM_Parse(&p), "one");
	OA_ASSERT_STR(COM_Parse(&p), "two");
	OA_ASSERT_STR(COM_Parse(&p), "three");
	OA_ASSERT_STR(COM_Parse(&p), "four");
	OA_ASSERT_STR(COM_Parse(&p), "");
}

TEST(com_parse_whitespace_only)
{
	char buf[] = "   \t  \n  ";
	char *p = buf;
	OA_ASSERT_STR(COM_Parse(&p), "");
}

TEST(com_parse_escaped_quote_inside)
{
	/* a quote followed by another quote ends the token; the second quote
	 * starts a new (empty) token. Assert the actual engine behaviour. */
	char buf[] = "\"a\"\"b\"";
	char *p = buf;
	OA_ASSERT_STR(COM_Parse(&p), "a");
	OA_ASSERT_STR(COM_Parse(&p), "b");
}

TEST(com_parse_comma_separated)
{
	char buf[] = "x,y,z";
	char *p = buf;
	/* COM_Parse treats commas as token characters, so the whole thing is
	 * one token unless separated by whitespace */
	OA_ASSERT_STR(COM_Parse(&p), "x,y,z");
	OA_ASSERT_STR(COM_Parse(&p), "");
}
