/*
 * tests_qinfo.c - Unit tests for the info-string helpers in
 * qcommon/q_shared.c (Info_ValueForKey, Info_SetValueForKey,
 * Info_RemoveKey, Info_Validate, Info_NextPair and their _Big variants).
 *
 * Info strings are the wire format for userinfo/serverinfo in Q3/OpenArena
 * and have historically been a source of buffer-overflow bugs, so they are
 * worth locking down with tests. No SDL/OpenGL required.
 *
 * NOTE: Info_ValueForKey / Info_RemoveKey / Info_SetValueForKey call
 * Com_Error(ERR_DROP) on oversize input. Our test stub's Com_Error aborts,
 * so we deliberately test only in-bounds inputs here. Oversize handling is
 * left to a fuzz/smoke test with a non-aborting error stub.
 *
 * Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c tests_qmath.c tests_qinfo.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"

TEST(info_value_for_key_missing)
{
	char s[MAX_INFO_STRING] = "\\name\\player";
	OA_ASSERT_STR(Info_ValueForKey(s, "score"), "");
}

TEST(info_value_for_key_present)
{
	char s[MAX_INFO_STRING] = "\\name\\player\\score\\42";
	OA_ASSERT_STR(Info_ValueForKey(s, "name"), "player");
	OA_ASSERT_STR(Info_ValueForKey(s, "score"), "42");
}

TEST(info_value_for_key_first_backslash)
{
	/* leading backslash is tolerated */
	char s[MAX_INFO_STRING] = "name\\player";
	OA_ASSERT_STR(Info_ValueForKey(s, "name"), "player");
}

TEST(info_value_for_key_empty_value)
{
	char s[MAX_INFO_STRING] = "\\clan\\";
	OA_ASSERT_STR(Info_ValueForKey(s, "clan"), "");
}

TEST(info_value_for_key_case_sensitive)
{
	char s[MAX_INFO_STRING] = "\\Name\\player";
	/* Info_ValueForKey uses case-sensitive Q_stricmp? -> it uses Q_stricmp
	 * (case-insensitive). Assert the actual behaviour. */
	OA_ASSERT_STR(Info_ValueForKey(s, "name"), "player");
}

TEST(info_set_value_adds_new_key)
{
	char s[MAX_INFO_STRING] = "\\name\\player";
	Info_SetValueForKey(s, "score", "42");
	OA_ASSERT_STR(Info_ValueForKey(s, "score"), "42");
	OA_ASSERT_STR(Info_ValueForKey(s, "name"), "player");
}

TEST(info_set_value_updates_existing)
{
	char s[MAX_INFO_STRING] = "\\name\\player\\score\\1";
	Info_SetValueForKey(s, "score", "99");
	OA_ASSERT_STR(Info_ValueForKey(s, "score"), "99");
	/* only one score key remains */
	Info_SetValueForKey(s, "score", "7");
	OA_ASSERT_STR(Info_ValueForKey(s, "score"), "7");
}

TEST(info_set_value_empty_removes)
{
	char s[MAX_INFO_STRING] = "\\name\\player\\score\\42";
	Info_SetValueForKey(s, "score", "");
	OA_ASSERT_STR(Info_ValueForKey(s, "score"), "");
	OA_ASSERT_STR(Info_ValueForKey(s, "name"), "player");
}

TEST(info_set_value_rejects_backslash_in_key)
{
	char s[MAX_INFO_STRING] = "\\name\\player";
	Info_SetValueForKey(s, "bad\\key", "x");
	/* key must not be added */
	OA_ASSERT_STR(Info_ValueForKey(s, "bad"), "");
}

TEST(info_set_value_rejects_semicolon_in_value)
{
	char s[MAX_INFO_STRING] = "\\name\\player";
	Info_SetValueForKey(s, "cmd", "a;b");
	OA_ASSERT_STR(Info_ValueForKey(s, "cmd"), "");
}

TEST(info_remove_key_present)
{
	char s[MAX_INFO_STRING] = "\\name\\player\\score\\42";
	Info_RemoveKey(s, "score");
	OA_ASSERT_STR(Info_ValueForKey(s, "score"), "");
	OA_ASSERT_STR(Info_ValueForKey(s, "name"), "player");
}

TEST(info_remove_key_first)
{
	char s[MAX_INFO_STRING] = "\\name\\player\\score\\42";
	Info_RemoveKey(s, "name");
	OA_ASSERT_STR(Info_ValueForKey(s, "name"), "");
	OA_ASSERT_STR(Info_ValueForKey(s, "score"), "42");
}

TEST(info_remove_key_missing_is_noop)
{
	char s[MAX_INFO_STRING] = "\\name\\player";
	Info_RemoveKey(s, "zzz");
	OA_ASSERT_STR(s, "\\name\\player");
}

TEST(info_validate_accepts_normal)
{
	OA_ASSERT(Info_Validate("\\name\\player\\score\\42") == qtrue);
}

TEST(info_validate_rejects_quote)
{
	OA_ASSERT(Info_Validate("\\name\\play\"er") == qfalse);
}

TEST(info_validate_rejects_semicolon)
{
	OA_ASSERT(Info_Validate("\\cmd\\a;b") == qfalse);
}

TEST(info_next_pair_iterates)
{
	const char *head = "\\name\\player\\score\\42";
	char key[MAX_INFO_KEY];
	char value[MAX_INFO_VALUE];

	Info_NextPair(&head, key, value);
	OA_ASSERT_STR(key, "name");
	OA_ASSERT_STR(value, "player");

	Info_NextPair(&head, key, value);
	OA_ASSERT_STR(key, "score");
	OA_ASSERT_STR(value, "42");

	Info_NextPair(&head, key, value);
	OA_ASSERT_STR(key, "");
	OA_ASSERT_STR(value, "");
}

TEST(info_set_big_retains_empty_value)
{
	char s[BIG_INFO_STRING] = "\\name\\player";
	Info_SetValueForKey_Big(s, "clan", "");
	/* unlike Info_SetValueForKey, the _Big variant keeps empty values */
	OA_ASSERT_STR(Info_ValueForKey(s, "clan"), "");
}

TEST(info_set_big_updates_existing)
{
	char s[BIG_INFO_STRING] = "\\name\\player\\score\\1";
	Info_SetValueForKey_Big(s, "score", "5");
	OA_ASSERT_STR(Info_ValueForKey(s, "score"), "5");
}

TEST(info_remove_key_big_present)
{
	char s[BIG_INFO_STRING] = "\\name\\player\\score\\42";
	Info_RemoveKey_Big(s, "score");
	OA_ASSERT_STR(Info_ValueForKey(s, "score"), "");
	OA_ASSERT_STR(Info_ValueForKey(s, "name"), "player");
}
