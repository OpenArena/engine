/*
 * tests_qchar.c - Unit tests for character-classification and numeric
 * predicate helpers in qcommon/q_shared.c: Q_isprint, Q_islower, Q_isupper,
 * Q_isalpha, Q_isanumber, Q_isintegral.
 *
 * All SDL/OpenGL-free. Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c tests_qmath.c tests_qinfo.c \
 *       tests_qstr.c tests_qmatrix.c tests_qtoken.c tests_qchar.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"

TEST(q_isprint_basic)
{
	OA_ASSERT_INT(Q_isprint('a'), 1);
	OA_ASSERT_INT(Q_isprint('Z'), 1);
	OA_ASSERT_INT(Q_isprint(' '), 1);   /* 0x20 */
	OA_ASSERT_INT(Q_isprint('~'), 1);   /* 0x7E */
}

TEST(q_isprint_control_rejected)
{
	OA_ASSERT_INT(Q_isprint(0x01), 0);
	OA_ASSERT_INT(Q_isprint(0x7F), 0);  /* DEL is outside the 0x20..0x7E range */
	OA_ASSERT_INT(Q_isprint(0x80), 0);
}

TEST(q_islower_basic)
{
	OA_ASSERT_INT(Q_islower('a'), 1);
	OA_ASSERT_INT(Q_islower('z'), 1);
	OA_ASSERT_INT(Q_islower('A'), 0);
	OA_ASSERT_INT(Q_islower('1'), 0);
	OA_ASSERT_INT(Q_islower(' '), 0);
}

TEST(q_isupper_basic)
{
	OA_ASSERT_INT(Q_isupper('A'), 1);
	OA_ASSERT_INT(Q_isupper('Z'), 1);
	OA_ASSERT_INT(Q_isupper('a'), 0);
	OA_ASSERT_INT(Q_isupper('1'), 0);
}

TEST(q_isalpha_basic)
{
	OA_ASSERT_INT(Q_isalpha('a'), 1);
	OA_ASSERT_INT(Q_isalpha('Z'), 1);
	OA_ASSERT_INT(Q_isalpha('5'), 0);
	OA_ASSERT_INT(Q_isalpha(' '), 0);
}

TEST(q_isanumber_integer)
{
	OA_ASSERT(Q_isanumber("123") == qtrue);
	OA_ASSERT(Q_isanumber("-42") == qtrue);
	OA_ASSERT(Q_isanumber("0") == qtrue);
}

TEST(q_isanumber_float)
{
	OA_ASSERT(Q_isanumber("3.14") == qtrue);
	OA_ASSERT(Q_isanumber("-0.5") == qtrue);
	OA_ASSERT(Q_isanumber("1e3") == qtrue);
}

TEST(q_isanumber_rejects_nonnumeric)
{
	OA_ASSERT(Q_isanumber("abc") == qfalse);
	OA_ASSERT(Q_isanumber("") == qfalse);
	OA_ASSERT(Q_isanumber("12a") == qfalse);
	OA_ASSERT(Q_isanumber("1.2.3") == qfalse);
}

TEST(q_isintegral_true)
{
	OA_ASSERT(Q_isintegral(0.0f) == qtrue);
	OA_ASSERT(Q_isintegral(5.0f) == qtrue);
	OA_ASSERT(Q_isintegral(-3.0f) == qtrue);
}

TEST(q_isintegral_false)
{
	OA_ASSERT(Q_isintegral(0.5f) == qfalse);
	OA_ASSERT(Q_isintegral(3.14f) == qfalse);
}

TEST(q_isintegral_large_float)
{
	/* a float that is exactly an integer value still counts as integral */
	OA_ASSERT(Q_isintegral(1000000.0f) == qtrue);
}
