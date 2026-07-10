/*
 * tests_qmatrix.c - Unit tests for matrix/axis transforms in
 * qcommon/q_math.c: AnglesToAxis, AxisClear, AxisCopy, Q_MatrixMultiply,
 * VectorRotate. These are on the transform pipeline for rendering; pure
 * float math, no SDL/OpenGL, so they run in CI.
 *
 * Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c tests_qmath.c tests_qinfo.c \
 *       tests_qstr.c tests_qmatrix.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"

#define EPS 1e-4f

TEST(angles_to_axis_identity)
{
	vec3_t axis[3];
	vec3_t ang = {0, 0, 0};
	AnglesToAxis(ang, axis);
	/* axis[0]=forward (AngleVectors convention at yaw0/pitch0 = +X),
	 * axis[1]=-right = +Y, axis[2]=up = +Z */
	OA_ASSERT_VEC3(axis[0], ((vec3_t){1, 0, 0}), EPS);
	OA_ASSERT_VEC3(axis[1], ((vec3_t){0, 1, 0}), EPS);
	OA_ASSERT_VEC3(axis[2], ((vec3_t){0, 0, 1}), EPS);
}

TEST(angles_to_axis_is_orthonormal)
{
	vec3_t axis[3];
	vec3_t ang = {12, 47, 0};
	AnglesToAxis(ang, axis);
	for (int i = 0; i < 3; i++)
		VectorNormalize(axis[i]);
	OA_ASSERT_FLOAT(DotProduct(axis[0], axis[1]), 0.0f, 1e-3f);
	OA_ASSERT_FLOAT(DotProduct(axis[1], axis[2]), 0.0f, 1e-3f);
	OA_ASSERT_FLOAT(DotProduct(axis[0], axis[2]), 0.0f, 1e-3f);
}

TEST(axis_clear_is_identity)
{
	vec3_t axis[3];
	AxisClear(axis);
	OA_ASSERT_VEC3(axis[0], ((vec3_t){1, 0, 0}), EPS);
	OA_ASSERT_VEC3(axis[1], ((vec3_t){0, 1, 0}), EPS);
	OA_ASSERT_VEC3(axis[2], ((vec3_t){0, 0, 1}), EPS);
}

TEST(axis_copy_deep)
{
	vec3_t src[3] = { {1, 2, 3}, {4, 5, 6}, {7, 8, 9} };
	vec3_t dst[3];
	AxisCopy(src, dst);
	for (int i = 0; i < 3; i++)
		OA_ASSERT_VEC3(dst[i], src[i], EPS);
	/* dst is independent of src */
	dst[0][0] = 99;
	OA_ASSERT_FLOAT(src[0][0], 1.0f, EPS);
}

TEST(matrix_multiply_identity_left)
{
	float I[3][3] = { {1,0,0}, {0,1,0}, {0,0,1} };
	float A[3][3] = { {1,2,3}, {4,5,6}, {7,8,9} };
	float out[3][3];
	Q_MatrixMultiply(I, A, out);
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			OA_ASSERT_FLOAT(out[i][j], A[i][j], EPS);
}

TEST(matrix_multiply_identity_right)
{
	float I[3][3] = { {1,0,0}, {0,1,0}, {0,0,1} };
	float A[3][3] = { {1,2,3}, {4,5,6}, {7,8,9} };
	float out[3][3];
	Q_MatrixMultiply(A, I, out);
	for (int i = 0; i < 3; i++)
		for (int j = 0; j < 3; j++)
			OA_ASSERT_FLOAT(out[i][j], A[i][j], EPS);
}

TEST(matrix_multiply_known_product)
{
	/* A = [[1,2,0],[0,1,0],[0,0,1]], B = [[1,0,0],[3,1,0],[0,0,1]]
	 * A*B = [[7,2,0],[3,1,0],[0,0,1]] */
	float A[3][3] = { {1,2,0}, {0,1,0}, {0,0,1} };
	float B[3][3] = { {1,0,0}, {3,1,0}, {0,0,1} };
	float out[3][3];
	Q_MatrixMultiply(A, B, out);
	OA_ASSERT_FLOAT(out[0][0], 7.0f, EPS);
	OA_ASSERT_FLOAT(out[0][1], 2.0f, EPS);
	OA_ASSERT_FLOAT(out[1][0], 3.0f, EPS);
	OA_ASSERT_FLOAT(out[1][1], 1.0f, EPS);
}

TEST(matrix_multiply_not_commutative)
{
	float A[3][3] = { {1,2,3}, {0,1,0}, {0,0,1} };
	float B[3][3] = { {1,0,0}, {0,2,0}, {0,0,1} };
	float ab[3][3], ba[3][3];
	Q_MatrixMultiply(A, B, ab);
	Q_MatrixMultiply(B, A, ba);
	/* ab != ba in general */
	OA_ASSERT_FLOAT(ab[0][1], 4.0f, EPS); /* 1*0+2*2+3*0 */
	OA_ASSERT_FLOAT(ba[0][1], 2.0f, EPS); /* 1*2+0*1+0*0 */
}

TEST(vector_rotate_identity_axis)
{
	/* rotating by the identity axis matrix leaves the vector unchanged */
	vec3_t axis[3];
	AxisClear(axis);
	vec3_t v = {1, 2, 3}, out;
	VectorRotate(v, axis, out);
	OA_ASSERT_VEC3(out, v, EPS);
}

TEST(vector_rotate_x_by_90_about_z)
{
	/* VectorRotate uses axis[i] as the i-th basis ROW, i.e. out = axis^T * v.
	 * With axis[0]={0,1,0}, axis[1]={-1,0,0}, axis[2]={0,0,1} and v={1,0,0}:
	 *   out[0] = Dot(v,axis[0]) = 0
	 *   out[1] = Dot(v,axis[1]) = -1
	 *   out[2] = Dot(v,axis[2]) = 0  -> (0,-1,0) */
	vec3_t axis[3] = { {0, 1, 0}, {-1, 0, 0}, {0, 0, 1} };
	vec3_t v = {1, 0, 0}, out;
	VectorRotate(v, axis, out);
	OA_ASSERT_VEC3(out, ((vec3_t){0, -1, 0}), 1e-3f);
}
