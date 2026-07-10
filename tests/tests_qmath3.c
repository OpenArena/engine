/*
 * tests_qmath3.c - More unit tests for qcommon/q_math.c: NormalizeColor,
 * PlaneFromPoints, Matrix4Compare / Matrix4Copy / Matrix4Multiply,
 * MakeNormalVectors, vectoangles (inverse of AngleVectors), Q_acos.
 *
 * All SDL/OpenGL-free. Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I.. -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c tests_qmath.c tests_qinfo.c \
 *       tests_qstr.c tests_qmatrix.c tests_qtoken.c tests_qchar.c \
 *       tests_qhuffman.c tests_qmsg.c tests_qformat.c tests_qmath2.c tests_qmath3.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/huffman.c ../code/qcommon/msg.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"

TEST(normalize_color_identity)
{
	vec3_t in = { 1, 1, 1 }, out;
	float m = NormalizeColor(in, out);
	OA_ASSERT_FLOAT(m, 1.0f, 1e-5f);
	OA_ASSERT_VEC3(out, ((vec3_t){1, 1, 1}), 1e-5f);
}

TEST(normalize_color_scales_by_max)
{
	vec3_t in = { 0.2f, 0.4f, 0.8f }, out;
	float m = NormalizeColor(in, out);
	OA_ASSERT_FLOAT(m, 0.8f, 1e-5f);
	OA_ASSERT_VEC3(out, ((vec3_t){0.25f, 0.5f, 1.0f}), 1e-5f);
}

TEST(normalize_color_black)
{
	vec3_t in = { 0, 0, 0 }, out;
	float m = NormalizeColor(in, out);
	OA_ASSERT_FLOAT(m, 0.0f, 1e-5f);
	OA_ASSERT_VEC3(out, ((vec3_t){0, 0, 0}), 1e-5f);
}

TEST(plane_from_points_valid)
{
	/* right-handed triangle in the XY plane.
	 * Engine computes CrossProduct(d2, d1) with d1=(b-a), d2=(c-a):
	 *   (0,1,0) x (1,0,0) = (0,0,-1)  -> normal is -Z here. */
	vec3_t a = { 0, 0, 0 }, b = { 1, 0, 0 }, c = { 0, 1, 0 };
	vec4_t plane;
	qboolean ok = PlaneFromPoints(plane, a, b, c);
	OA_ASSERT(ok == qtrue);
	OA_ASSERT_VEC3(plane, ((vec4_t){0, 0, -1, 0}), 1e-5f);
}

TEST(plane_from_points_degenerate)
{
	/* three collinear points -> degenerate -> qfalse */
	vec3_t a = { 0, 0, 0 }, b = { 1, 0, 0 }, c = { 2, 0, 0 };
	vec4_t plane;
	qboolean ok = PlaneFromPoints(plane, a, b, c);
	OA_ASSERT(ok == qfalse);
}

TEST(matrix4_compare)
{
	float a[16], b[16];
	for (int i = 0; i < 16; i++) { a[i] = (float)i; b[i] = (float)i; }
	OA_ASSERT(Matrix4Compare(a, b) == qtrue);
	b[5] = 99.0f;
	OA_ASSERT(Matrix4Compare(a, b) == qfalse);
}

TEST(matrix4_copy)
{
	float a[16], b[16];
	for (int i = 0; i < 16; i++) { a[i] = (float)(i * 2); b[i] = 0; }
	Matrix4Copy(a, b);
	OA_ASSERT(Matrix4Compare(a, b) == qtrue);
}

TEST(matrix4_multiply_identity)
{
	float id[16] = {
		1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,1
	};
	float m[16] = {
		1,2,3,4, 5,6,7,8, 9,10,11,12, 13,14,15,16
	};
	float out[16];
	Matrix4Multiply(m, id, out);
	OA_ASSERT(Matrix4Compare(out, m) == qtrue);
	Matrix4Multiply(id, m, out);
	OA_ASSERT(Matrix4Compare(out, m) == qtrue);
}

TEST(matrix4_multiply_known_product)
{
	/* two simple matrices with a hand-checkable product.
	 * NOTE: Matrix4Multiply(a, b, out) computes out = b * a (the engine
	 * multiplies in this order), so for
	 *   a = [[1,2],[3,4]], b = [[5,6],[7,8]]  (row-major 2x2 blocks)
	 *   out = b*a top-left = [[5*1+6*3, 5*2+6*4],[7*1+8*3, 7*2+8*4]]
	 *                     = [[23,34],[31,46]] */
	float a[16] = {
		1,2,0,0, 3,4,0,0, 0,0,1,0, 0,0,0,1
	};
	float b[16] = {
		5,6,0,0, 7,8,0,0, 0,0,1,0, 0,0,0,1
	};
	float out[16];
	Matrix4Multiply(a, b, out);
	OA_ASSERT_FLOAT(out[0], 23.0f, 1e-5f);
	OA_ASSERT_FLOAT(out[1], 34.0f, 1e-5f);
	OA_ASSERT_FLOAT(out[4], 31.0f, 1e-5f);
	OA_ASSERT_FLOAT(out[5], 46.0f, 1e-5f);
}

TEST(make_normal_vectors_orthonormal)
{
	vec3_t fwd = { 1, 0, 0 }, right, up;
	MakeNormalVectors(fwd, right, up);
	/* right and up must be unit length and orthogonal to forward */
	OA_ASSERT_FLOAT(VectorLength(right), 1.0f, 1e-5f);
	OA_ASSERT_FLOAT(VectorLength(up), 1.0f, 1e-5f);
	OA_ASSERT_FLOAT(DotProduct(right, fwd), 0.0f, 1e-5f);
	OA_ASSERT_FLOAT(DotProduct(up, fwd), 0.0f, 1e-5f);
	OA_ASSERT_FLOAT(DotProduct(right, up), 0.0f, 1e-5f);
}

TEST(vectoangles_inverse_of_anglevectors)
{
	/* AngleVectors(angles) -> forward; vectoangles(forward) -> angles back.
	 * vectoangles may return the pitch wrapped (e.g. -330 instead of 30),
	 * which is the same angle mod 360, so compare normalized values. */
	vec3_t angles = { 30, 60, 0 };
	vec3_t fwd, right, up;
	AngleVectors(angles, fwd, right, up);
	vec3_t back;
	vectoangles(fwd, back);
	/* pitch/yaw should round-trip (roll is not recoverable from a direction).
	 * Both pass through AngleNormalize360's fixed-point math, so allow a
	 * tolerance larger than a single normalization step (~1e-2). */
	OA_ASSERT_FLOAT(AngleNormalize360(back[0]), AngleNormalize360(angles[0]), 1e-2f);
	OA_ASSERT_FLOAT(AngleNormalize360(back[1]), AngleNormalize360(angles[1]), 1e-2f);
}

TEST(q_acos)
{
	OA_ASSERT_FLOAT(Q_acos(1.0f), 0.0f, 1e-5f);
	OA_ASSERT_FLOAT(Q_acos(0.0f), 1.5707963f, 1e-5f);   /* pi/2 */
	OA_ASSERT_FLOAT(Q_acos(-1.0f), 3.1415926f, 1e-5f);  /* pi */
}
