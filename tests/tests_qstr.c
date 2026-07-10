/*
 * tests_qstr.c - More unit tests for string helpers in qcommon/q_shared.c:
 * Q_CleanStr, Q_PrintStrlen (color-code aware), Com_HexStrToInt,
 * COM_GetExtension, COM_DefaultExtension, Com_TruncateLongString.
 *
 * All SDL/OpenGL-free. Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c tests_qmath.c tests_qinfo.c tests_qstr.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"

TEST(q_printstrlen_plain)
{
	OA_ASSERT_INT(Q_PrintStrlen("hello"), 5);
}

TEST(q_printstrlen_skips_color_codes)
{
	/* "^1" is a two-char color code (escape + digit); neither counts */
	OA_ASSERT_INT(Q_PrintStrlen("^1red^7white"), 8);
}

TEST(q_printstrlen_null_is_zero)
{
	OA_ASSERT_INT(Q_PrintStrlen(NULL), 0);
}

TEST(q_printstrlen_mixed)
{
	/* "ab^2cd" -> 'a','b','c','d' = 4 visible chars */
	OA_ASSERT_INT(Q_PrintStrlen("ab^2cd"), 4);
}

TEST(q_cleanstr_removes_color_codes)
{
	char s[] = "^1red^7white";
	Q_CleanStr(s);
	OA_ASSERT_STR(s, "redwhite");
}

TEST(q_cleanstr_keeps_plain_text)
{
	char s[] = "plain text";
	Q_CleanStr(s);
	OA_ASSERT_STR(s, "plain text");
}

TEST(q_cleanstr_drops_non_printable)
{
	/* control bytes < 0x20 are stripped (use an explicit array to avoid
	 * C hex-escape greediness in string literals) */
	char s[8];
	s[0] = 'a'; s[1] = 1; s[2] = 2; s[3] = 'b'; s[4] = 0;
	Q_CleanStr(s);
	OA_ASSERT_STR(s, "ab");
}

TEST(q_cleanstr_empty_stays_empty)
{
	char s[] = "";
	Q_CleanStr(s);
	OA_ASSERT_STR(s, "");
}

TEST(com_hexstrtoint_decimal_rejected)
{
	/* Com_HexStrToInt only parses 0x-prefixed hex; a bare decimal string
	 * (no 0x) returns -1, not the decimal value */
	OA_ASSERT_INT(Com_HexStrToInt("255"), -1);
}

TEST(com_hexstrtoint_hex_lower)
{
	OA_ASSERT_INT(Com_HexStrToInt("0xff"), 255);
}

TEST(com_hexstrtoint_hex_upper)
{
	OA_ASSERT_INT(Com_HexStrToInt("0xFF"), 255);
}

TEST(com_hexstrtoint_hex_alpha)
{
	OA_ASSERT_INT(Com_HexStrToInt("0x1a"), 26);
}

TEST(com_hexstrtoint_empty_is_neg1)
{
	OA_ASSERT_INT(Com_HexStrToInt(""), -1);
	OA_ASSERT_INT(Com_HexStrToInt(NULL), -1);
}

TEST(com_get_extension_basic)
{
	OA_ASSERT_STR(COM_GetExtension("map.bsp"), "bsp");
}

TEST(com_get_extension_with_path)
{
	OA_ASSERT_STR(COM_GetExtension("/a/b/models/player.md3"), "md3");
}

TEST(com_get_extension_none)
{
	OA_ASSERT_STR(COM_GetExtension("readme"), "");
}

TEST(com_get_extension_double_dot)
{
	/* extension is the part after the LAST dot */
	OA_ASSERT_STR(COM_GetExtension("archive.tar.gz"), "gz");
}

TEST(com_default_extension_adds_when_missing)
{
	char out[64];
	strcpy(out, "map");
	COM_DefaultExtension(out, sizeof(out), ".bsp");
	OA_ASSERT_STR(out, "map.bsp");
}

TEST(com_default_extension_keeps_existing)
{
	char out[64];
	strcpy(out, "map.bsp");
	COM_DefaultExtension(out, sizeof(out), ".bsp");
	OA_ASSERT_STR(out, "map.bsp");
}

TEST(com_truncate_short_string_unchanged)
{
	char buf[TRUNCATE_LENGTH + 1];
	Com_TruncateLongString(buf, "short");
	OA_ASSERT_STR(buf, "short");
}

TEST(com_truncate_long_string_ellipsizes)
{
	char buf[TRUNCATE_LENGTH + 1];
	char longstr[TRUNCATE_LENGTH * 2];
	memset(longstr, 'a', sizeof(longstr) - 1);
	longstr[sizeof(longstr) - 1] = 0;
	Com_TruncateLongString(buf, longstr);
	/* truncated form keeps head+tail with a " ... " separator and never
	 * exceeds TRUNCATE_LENGTH */
	OA_ASSERT((int)strlen(buf) <= TRUNCATE_LENGTH);
	OA_ASSERT((int)strlen(buf) > TRUNCATE_LENGTH / 2);
	OA_ASSERT_INT(strncmp(buf, "aaa", 3), 0);
	OA_ASSERT(strstr(buf, " ... ") != NULL);
}
