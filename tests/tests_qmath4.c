/*
 * tests_qmath4.c - More q_math.c coverage: PerpendicularVector,
 * RotateAroundDirection, Q_fabs. These complement the earlier q_math suites.
 *
 * All SDL/OpenGL-free. Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I.. -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c tests_qmath.c tests_qinfo.c \
 *       tests_qstr.c tests_qmatrix.c tests_qtoken.c tests_qchar.c \
 *       tests_qhuffman.c tests_qmsg.c tests_qformat.c tests_qmath2.c \
 *       tests_qmath3.c tests_qstring2.c tests_qmsg2.c tests_qmsg3.c \
 *       tests_qmath4.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/huffman.c ../code/qcommon/msg.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"

TEST(perpendicular_vector_is_orthogonal)
{
	/* PerpendicularVector expects a NORMALIZED source (ProjectPointOnPlane
	 * uses the normal as-is, so a unit normal is required for a unit,
	 * orthogonal result). */
	vec3_t src = { 1, 2, 3 };
	VectorNormalize(src);
	vec3_t dst;
	PerpendicularVector(dst, src);
	OA_ASSERT_FLOAT(DotProduct(dst, src), 0.0f, 1e-5f);
	OA_ASSERT_FLOAT(VectorLength(dst), 1.0f, 1e-5f);
}

TEST(perpendicular_vector_axis_aligned)
{
	/* for an axis-aligned (already unit) source, the result is well defined:
	 * PerpendicularVector({0,0,1}) projects {1,0,0} onto the plane normal to
	 * Z, yielding {1,0,0} normalized. */
	vec3_t src = { 0, 0, 1 };
	vec3_t dst;
	PerpendicularVector(dst, src);
	OA_ASSERT_VEC3(dst, ((vec3_t){1, 0, 0}), 1e-5f);
}

TEST(perpendicular_vector_negative)
{
	vec3_t src = { -1, -1, -1 };
	VectorNormalize(src);
	vec3_t dst;
	PerpendicularVector(dst, src);
	OA_ASSERT_FLOAT(DotProduct(dst, src), 0.0f, 1e-5f);
	OA_ASSERT_FLOAT(VectorLength(dst), 1.0f, 1e-5f);
}

TEST(rotate_around_direction_forward_preserved)
{
	/* axis[0] is the INPUT forward vector (must be set/normalized by caller).
	 * RotateAroundDirection fills axis[1] and axis[2]; axis[0] is unchanged. */
	vec3_t axis[3];
	axis[0][0] = 1; axis[0][1] = 0; axis[0][2] = 0;  /* already unit */
	RotateAroundDirection(axis, 0.0f);
	OA_ASSERT_VEC3(axis[0], ((vec3_t){1, 0, 0}), 1e-5f);
}

TEST(rotate_around_direction_columns_orthonormal)
{
	vec3_t axis[3];
	axis[0][0] = 0; axis[0][1] = 0; axis[0][2] = 1;  /* unit forward */
	RotateAroundDirection(axis, 37.0f);
	/* the three columns form an orthonormal basis, each unit length and
	 * mutually orthogonal */
	OA_ASSERT_FLOAT(VectorLength(axis[0]), 1.0f, 1e-5f);
	OA_ASSERT_FLOAT(VectorLength(axis[1]), 1.0f, 1e-5f);
	OA_ASSERT_FLOAT(VectorLength(axis[2]), 1.0f, 1e-5f);
	OA_ASSERT_FLOAT(DotProduct(axis[0], axis[1]), 0.0f, 1e-5f);
	OA_ASSERT_FLOAT(DotProduct(axis[1], axis[2]), 0.0f, 1e-5f);
	OA_ASSERT_FLOAT(DotProduct(axis[0], axis[2]), 0.0f, 1e-5f);
}

TEST(q_fabs)
{
	OA_ASSERT_FLOAT(Q_fabs(-3.5f), 3.5f, 1e-6f);
	OA_ASSERT_FLOAT(Q_fabs(3.5f), 3.5f, 1e-6f);
	OA_ASSERT_FLOAT(Q_fabs(0.0f), 0.0f, 1e-6f);
	OA_ASSERT_FLOAT(Q_fabs(-0.0f), 0.0f, 1e-6f);
}
