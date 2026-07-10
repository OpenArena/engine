/*
 * tests_qshared.c - Unit tests for qcommon/q_shared.c string & parse helpers.
 *
 * Build & run (standalone, no SDL/OpenGL needed):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_qshared \
 *       oa_test.h oa_test_run.c tests_qshared.c ../code/qcommon/q_shared.c \
 *       ../code/qcommon/q_math.c 2>&1
 *   ./test_qshared
 */
#include "oa_test.h"

#include "q_shared.h"

/* Com_Error would abort the process; for string tests we cannot easily trap
 * it. NULL-dest / NULL-src paths call Com_Error(ERR_FATAL) which longjmps via
 * the engine's error handler. We test the safe, non-aborting behaviours and
 * document the fatal ones as known constraints. To keep the suite runnable we
 * avoid the fatal inputs. */

TEST(q_strncpyz_basic_copy)
{
	char dst[32];
	Q_strncpyz(dst, "hello", sizeof(dst));
	OA_ASSERT_STR(dst, "hello");
}

TEST(q_strncpyz_exact_fit_includes_null)
{
	char dst[6];
	Q_strncpyz(dst, "hello", sizeof(dst));
	OA_ASSERT_STR(dst, "hello");
	OA_ASSERT_INT(dst[5], 0);
}

TEST(q_strncpyz_truncates_long_source)
{
	char dst[4];
	Q_strncpyz(dst, "abcdef", sizeof(dst));
	/* destsize=4 -> 3 chars + NUL */
	OA_ASSERT_STRN(dst, "abc", 3);
	OA_ASSERT_INT(dst[3], 0);
}

TEST(q_strncpyz_empty_source)
{
	char dst[16];
	strcpy(dst, "garbage");
	Q_strncpyz(dst, "", sizeof(dst));
	OA_ASSERT_STR(dst, "");
}

TEST(q_stricmpn_case_insensitive)
{
	OA_ASSERT_INT(Q_stricmpn("HelloWorld", "helloworld", 5), 0);
	OA_ASSERT_INT(Q_stricmpn("HelloWorld", "helloworld", 11), 0);
}

TEST(q_stricmpn_respects_length)
{
	/* length 5 => only "Hello" vs "hello" compared, rest ignored */
	OA_ASSERT_INT(Q_stricmpn("HelloX", "helloY", 5), 0);
	/* length 6 => 'X' vs 'Y' differ (case-insensitive only, not
	 * substitute-different-letters). Must be non-zero. */
	OA_ASSERT(Q_stricmpn("HelloX", "helloY", 6) != 0);
}

TEST(q_stricmpn_difference_detected)
{
	OA_ASSERT(Q_stricmpn("abc", "abd", 3) != 0);
}

TEST(q_stricmp_null_safe)
{
	/* one NULL, one not -> -1 (see q_shared.c impl) */
	OA_ASSERT_INT(Q_stricmp(NULL, "x"), -1);
	OA_ASSERT_INT(Q_stricmp("x", NULL), -1);
}

TEST(com_sprintf_basic)
{
	char buf[32];
	Com_sprintf(buf, sizeof(buf), "value=%d", 42);
	OA_ASSERT_STR(buf, "value=42");
}

TEST(com_sprintf_truncation_guarded)
{
	char buf[6];
	Com_sprintf(buf, sizeof(buf), "abcdefghij");
	OA_ASSERT_STRN(buf, "abcde", 5);
	OA_ASSERT_INT(buf[5], 0);
}

TEST(com_sprintf_multiple_args)
{
	char buf[64];
	Com_sprintf(buf, sizeof(buf), "%s:%d:%s", "host", 27960, "name");
	OA_ASSERT_STR(buf, "host:27960:name");
}

TEST(com_strip_extension_none)
{
	char out[64];
	COM_StripExtension("readme", out, sizeof(out));
	OA_ASSERT_STR(out, "readme");
}

TEST(com_strip_extension_basic)
{
	char out[64];
	COM_StripExtension("map.bsp", out, sizeof(out));
	OA_ASSERT_STR(out, "map");
}

TEST(com_strip_extension_keep_path)
{
	char out[128];
	COM_StripExtension("/a/b/models/player.md3", out, sizeof(out));
	OA_ASSERT_STR(out, "/a/b/models/player");
}

TEST(com_strip_extension_double_dot)
{
	char out[128];
	COM_StripExtension("archive.tar.gz", out, sizeof(out));
	OA_ASSERT_STR(out, "archive.tar");
}

TEST(com_skip_path_root)
{
	OA_ASSERT_STR(COM_SkipPath("/usr/local/bin/oa"), "oa");
}

TEST(com_skip_path_windows_style)
{
	/* COM_SkipPath only recognises '/' as a path separator (documented
	 * engine behaviour; backslash paths are normalised elsewhere). Assert
	 * the ACTUAL behaviour so a regression in the '/' handling is caught. */
	OA_ASSERT_STR(COM_SkipPath("C:\\games\\oa\\pak0.pk3"), "C:\\games\\oa\\pak0.pk3");
}

TEST(com_skip_path_no_slash)
{
	OA_ASSERT_STR(COM_SkipPath("justafile"), "justafile");
}

TEST(com_compress_removes_comments)
{
	char buf[256];
	strcpy(buf, "// a comment\nreal line\n");
	int len = COM_Compress(buf);
	OA_ASSERT(len > 0);
	/* the leading // comment line is stripped, but the newline that followed
	 * it is preserved (COM_Compress keeps one newline between remaining
	 * tokens). Result is "\nreal line". */
	OA_ASSERT_STR(buf, "\nreal line");
}

TEST(com_compress_preserves_content)
{
	char buf[256];
	strcpy(buf, "alpha beta gamma");
	COM_Compress(buf);
	OA_ASSERT_STR(buf, "alpha beta gamma");
}

TEST(q_count_char_basic)
{
	OA_ASSERT_INT(Q_CountChar("a/b/c/d", '/'), 3);
}

TEST(q_count_char_none)
{
	OA_ASSERT_INT(Q_CountChar("nopipes", '|'), 0);
}

TEST(q_count_char_empty)
{
	OA_ASSERT_INT(Q_CountChar("", 'x'), 0);
}

TEST(q_strlwr_basic)
{
	char s[16];
	strcpy(s, "HeLLo");
	Q_strlwr(s);
	OA_ASSERT_STR(s, "hello");
}

TEST(q_strupr_basic)
{
	char s[16];
	strcpy(s, "HeLLo");
	Q_strupr(s);
	OA_ASSERT_STR(s, "HELLO");
}

TEST(q_strcat_appends)
{
	char dst[32];
	strcpy(dst, "foo");
	Q_strcat(dst, sizeof(dst), "bar");
	OA_ASSERT_STR(dst, "foobar");
}

TEST(q_strcat_respects_size)
{
	char dst[6];
	strcpy(dst, "foo");
	Q_strcat(dst, sizeof(dst), "barbaz");
	OA_ASSERT_STRN(dst, "fooba", 5);
	OA_ASSERT_INT(dst[5], 0);
}
