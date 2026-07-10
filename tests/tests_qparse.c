/*
 * tests_qparse.c - Unit tests for the token parser in qcommon/q_shared.c
 * (COM_Parse / COM_ParseExt). This parser is on the hot path for shader and
 * config file loading, so regressions here are high-impact.
 *
 * Build & run (combined with tests_qshared.c):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"

TEST(com_parse_simple_tokens)
{
	char buf[] = "foo bar baz";
	char *p = buf;
	OA_ASSERT_STR(COM_Parse(&p), "foo");
	OA_ASSERT_STR(COM_Parse(&p), "bar");
	OA_ASSERT_STR(COM_Parse(&p), "baz");
	OA_ASSERT_STR(COM_Parse(&p), ""); /* end of string */
}

TEST(com_parse_whitespace_handling)
{
	char buf[] = "  alpha\t\tbeta  \n gamma ";
	char *p = buf;
	OA_ASSERT_STR(COM_Parse(&p), "alpha");
	OA_ASSERT_STR(COM_Parse(&p), "beta");
	OA_ASSERT_STR(COM_Parse(&p), "gamma");
	OA_ASSERT_STR(COM_Parse(&p), "");
}

TEST(com_parse_quoted_string)
{
	char buf[] = "\"hello world\" unquoted";
	char *p = buf;
	OA_ASSERT_STR(COM_Parse(&p), "hello world");
	OA_ASSERT_STR(COM_Parse(&p), "unquoted");
}

TEST(com_parse_curly_braces_are_tokens)
{
	char buf[] = "{ block }";
	char *p = buf;
	OA_ASSERT_STR(COM_Parse(&p), "{");
	OA_ASSERT_STR(COM_Parse(&p), "block");
	OA_ASSERT_STR(COM_Parse(&p), "}");
}

TEST(com_parse_skips_line_comments)
{
	char buf[] = "// this is a comment\nreal token";
	char *p = buf;
	OA_ASSERT_STR(COM_Parse(&p), "real");
	OA_ASSERT_STR(COM_Parse(&p), "token");
}

TEST(com_parse_skips_block_comments)
{
	char buf[] = "/* block\n comment */ value";
	char *p = buf;
	OA_ASSERT_STR(COM_Parse(&p), "value");
}

TEST(com_parse_empty_input)
{
	char *p = NULL;
	OA_ASSERT_STR(COM_Parse(&p), "");
}

TEST(com_parse_single_token_then_eof)
{
	char buf[] = "only";
	char *p = buf;
	OA_ASSERT_STR(COM_Parse(&p), "only");
	OA_ASSERT_STR(COM_Parse(&p), "");
	OA_ASSERT_STR(COM_Parse(&p), "");
}

TEST(com_parse_advance_pointer)
{
	char buf[] = "one two";
	char *p = buf;
	COM_Parse(&p);
	/* after consuming "one", p should point at " two" (whitespace + token) */
	OA_ASSERT_STR(p, " two");
}

TEST(com_parse_embedded_quote_not_closing)
{
	char buf[] = "\"a\"\"b\"";
	char *p = buf;
	OA_ASSERT_STR(COM_Parse(&p), "a");
	OA_ASSERT_STR(COM_Parse(&p), "b");
}
