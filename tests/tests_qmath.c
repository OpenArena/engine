/*
 * tests_qmath.c - Unit tests for the vector/matrix math in
 * qcommon/q_math.c and the vec3 macros in qcommon/q_shared.h.
 *
 * These are pure float math, no SDL/OpenGL, so they run in CI. Use
 * OA_ASSERT_VEC3 / OA_ASSERT_FLOAT with a tolerance to avoid platform float
 * noise.
 *
 * Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c tests_qmath.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"

#define EPS 1e-4f

TEST(vec3_macro_add)
{
	vec3_t a = {1, 2, 3}, b = {4, 5, 6}, c;
	VectorAdd(a, b, c);
	OA_ASSERT_VEC3(c, ((vec3_t){5, 7, 9}), EPS);
}

TEST(vec3_macro_subtract)
{
	vec3_t a = {4, 5, 6}, b = {1, 2, 3}, c;
	VectorSubtract(a, b, c);
	OA_ASSERT_VEC3(c, ((vec3_t){3, 3, 3}), EPS);
}

TEST(vec3_macro_clear)
{
	vec3_t a = {1, 2, 3};
	VectorClear(a);
	OA_ASSERT_VEC3_ZERO(a, EPS);
}

TEST(vec3_macro_set)
{
	vec3_t a;
	VectorSet(a, 1, 2, 3);
	OA_ASSERT_VEC3(a, ((vec3_t){1, 2, 3}), EPS);
}

TEST(vec3_macro_copy)
{
	vec3_t a = {1, 2, 3}, b;
	VectorCopy(a, b);
	OA_ASSERT_VEC3(a, b, EPS);
}

TEST(vec3_macro_dotproduct_orthogonal)
{
	vec3_t x = {1, 0, 0}, y = {0, 1, 0};
	OA_ASSERT_FLOAT(DotProduct(x, y), 0.0f, EPS);
}

TEST(vec3_macro_dotproduct_parallel)
{
	vec3_t a = {1, 2, 3};
	OA_ASSERT_FLOAT(DotProduct(a, a), 14.0f, EPS);
}

TEST(vector_normalize_unit)
{
	vec3_t v = {1, 0, 0};
	float len = VectorNormalize(v);
	OA_ASSERT_FLOAT(len, 1.0f, EPS);
	OA_ASSERT_VEC3(v, ((vec3_t){1, 0, 0}), EPS);
}

TEST(vector_normalize_diagonal)
{
	vec3_t v = {1, 1, 1};
	float len = VectorNormalize(v);
	OA_ASSERT_FLOAT(len, 1.73205f, 1e-3f);
	OA_ASSERT_VEC3(v, ((vec3_t){0.57735f, 0.57735f, 0.57735f}), 1e-3f);
}

TEST(vector_normalize_zero_is_safe)
{
	vec3_t v = {0, 0, 0};
	float len = VectorNormalize(v);
	/* engine returns 0 and leaves the vector untouched (no div-by-zero) */
	OA_ASSERT_FLOAT(len, 0.0f, EPS);
	OA_ASSERT_VEC3_ZERO(v, EPS);
}

TEST(cross_product_standard_basis)
{
	vec3_t x = {1, 0, 0}, y = {0, 1, 0}, z;
	CrossProduct(x, y, z);
	OA_ASSERT_VEC3(z, ((vec3_t){0, 0, 1}), EPS);
}

TEST(cross_product_anti_commutative)
{
	vec3_t a = {1, 2, 3}, b = {4, 5, 6}, ab, ba;
	CrossProduct(a, b, ab);
	CrossProduct(b, a, ba);
	vec3_t neg = {-ab[0], -ab[1], -ab[2]};
	OA_ASSERT_VEC3(ba, neg, EPS);
}

TEST(angle_vectors_identity)
{
	vec3_t forward, right, up;
	vec3_t ang = {0, 0, 0};
	AngleVectors(ang, forward, right, up);
	/* Q3 convention at yaw=0,pitch=0: forward = +X, right = -Y, up = +Z */
	OA_ASSERT_VEC3(forward, ((vec3_t){1, 0, 0}), EPS);
	OA_ASSERT_VEC3(right, ((vec3_t){0, -1, 0}), EPS);
	OA_ASSERT_VEC3(up, ((vec3_t){0, 0, 1}), EPS);
}

TEST(angle_vectors_yaw_90)
{
	vec3_t forward, right, up;
	vec3_t ang = {0, 90, 0};
	AngleVectors(ang, forward, right, up);
	/* yaw 90: forward rotates from +X toward +Y -> (0,1,0); right -> (1,0,0) */
	OA_ASSERT_VEC3(forward, ((vec3_t){0, 1, 0}), 1e-3f);
	OA_ASSERT_VEC3(right, ((vec3_t){1, 0, 0}), 1e-3f);
}

TEST(angle_vectors_orthonormal)
{
	vec3_t forward, right, up;
	vec3_t ang = {12, 47, 0};
	AngleVectors(ang, forward, right, up);
	OA_ASSERT_FLOAT(VectorNormalize(forward), 1.0f, EPS);
	OA_ASSERT_FLOAT(VectorNormalize(right), 1.0f, EPS);
	OA_ASSERT_FLOAT(VectorNormalize(up), 1.0f, EPS);
	/* the three basis vectors must be mutually orthogonal */
	OA_ASSERT_FLOAT(DotProduct(forward, right), 0.0f, 1e-3f);
	OA_ASSERT_FLOAT(DotProduct(right, up), 0.0f, 1e-3f);
	OA_ASSERT_FLOAT(DotProduct(forward, up), 0.0f, 1e-3f);
}

TEST(project_point_on_plane_xy)
{
	vec3_t p = {3, 4, 5};
	vec3_t n = {0, 0, 1}; /* project onto XY plane */
	vec3_t out;
	ProjectPointOnPlane(out, p, n);
	/* z component should drop to 0, x/y preserved */
	OA_ASSERT_VEC3(out, ((vec3_t){3, 4, 0}), EPS);
}

TEST(project_point_on_plane_origin)
{
	vec3_t p = {1, 1, 1};
	vec3_t n = {1, 1, 1};
	VectorNormalize(n); /* ProjectPointOnPlane expects a unit normal */
	vec3_t out;
	ProjectPointOnPlane(out, p, n);
	/* projecting (1,1,1) onto the plane through the origin with normal
	 * (1,1,1)/sqrt(3) yields the origin; the result is orthogonal to n */
	OA_ASSERT_VEC3_ZERO(out, 1e-3f);
	OA_ASSERT_FLOAT(DotProduct(out, n), 0.0f, 1e-3f);
}

TEST(rotate_point_around_z_90)
{
	vec3_t dir = {0, 0, 1};
	vec3_t point = {1, 0, 0};
	vec3_t out;
	RotatePointAroundVector(out, dir, point, 90.0f);
	/* rotating (1,0,0) around +Z by 90deg -> (0,1,0) */
	OA_ASSERT_VEC3(out, ((vec3_t){0, 1, 0}), 1e-3f);
}

TEST(rotate_point_around_z_full_turn)
{
	vec3_t dir = {0, 0, 1};
	vec3_t point = {1, 0, 0};
	vec3_t out;
	RotatePointAroundVector(out, dir, point, 360.0f);
	OA_ASSERT_VEC3(out, ((vec3_t){1, 0, 0}), 1e-3f);
}

TEST(q_rsqrt_approx)
{
	/* Q_rsqrt is the fast inverse sqrt; within a few percent of 1/sqrt(x) */
	OA_ASSERT_FLOAT(Q_rsqrt(4.0f), 0.5f, 0.01f);
	OA_ASSERT_FLOAT(Q_rsqrt(2.0f), 0.7071f, 0.01f);
}

TEST(make_normal_vectors_orthogonal)
{
	vec3_t fwd = {1, 0, 0}, right, up;
	VectorNormalize(fwd);
	MakeNormalVectors(fwd, right, up);
	OA_ASSERT_FLOAT(VectorNormalize(right), 1.0f, EPS);
	OA_ASSERT_FLOAT(VectorNormalize(up), 1.0f, EPS);
	OA_ASSERT_FLOAT(DotProduct(fwd, right), 0.0f, 1e-3f);
	OA_ASSERT_FLOAT(DotProduct(fwd, up), 0.0f, 1e-3f);
	OA_ASSERT_FLOAT(DotProduct(right, up), 0.0f, 1e-3f);
}
