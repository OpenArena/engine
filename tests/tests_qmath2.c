/*
 * tests_qmath2.c - More unit tests for qcommon/q_math.c: bounding-box
 * helpers (ClearBounds, AddPointToBounds, BoundsIntersect,
 * BoundsIntersectSphere, BoundsIntersectPoint, RadiusFromBounds) plus angle
 * normalization (AngleNormalize360 / 180, AngleDelta, LerpAngle), the
 * DirToByte / ByteToDir round-trip, Q_log2, and Q_isnan.
 *
 * All SDL/OpenGL-free. Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I.. -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c tests_qmath.c tests_qinfo.c \
 *       tests_qstr.c tests_qmatrix.c tests_qtoken.c tests_qchar.c \
 *       tests_qhuffman.c tests_qmsg.c tests_qformat.c tests_qmath2.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/huffman.c ../code/qcommon/msg.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"

TEST(clearbounds_initializes_empty)
{
	vec3_t mins, maxs;
	ClearBounds(mins, maxs);
	OA_ASSERT_FLOAT(mins[0], 99999.0f, 1e-3f);
	OA_ASSERT_FLOAT(maxs[0], -99999.0f, 1e-3f);
}

TEST(addpointtobounds_single)
{
	vec3_t mins, maxs;
	ClearBounds(mins, maxs);
	vec3_t p = { 1, 2, 3 };
	AddPointToBounds(p, mins, maxs);
	OA_ASSERT_VEC3(mins, ((vec3_t){1, 2, 3}), 1e-3f);
	OA_ASSERT_VEC3(maxs, ((vec3_t){1, 2, 3}), 1e-3f);
}

TEST(addpointtobounds_expands)
{
	vec3_t mins, maxs;
	ClearBounds(mins, maxs);
	vec3_t a = { -1, 5, 0 };
	vec3_t b = { 4, -2, 10 };
	AddPointToBounds(a, mins, maxs);
	AddPointToBounds(b, mins, maxs);
	OA_ASSERT_VEC3(mins, ((vec3_t){-1, -2, 0}), 1e-3f);
	OA_ASSERT_VEC3(maxs, ((vec3_t){4, 5, 10}), 1e-3f);
}

TEST(boundsintersect_overlapping)
{
	vec3_t m1 = { 0, 0, 0 }, M1 = { 10, 10, 10 };
	vec3_t m2 = { 5, 5, 5 }, M2 = { 15, 15, 15 };
	OA_ASSERT(BoundsIntersect(m1, M1, m2, M2) == qtrue);
}

TEST(boundsintersect_disjoint)
{
	vec3_t m1 = { 0, 0, 0 }, M1 = { 10, 10, 10 };
	vec3_t m2 = { 20, 20, 20 }, M2 = { 30, 30, 30 };
	OA_ASSERT(BoundsIntersect(m1, M1, m2, M2) == qfalse);
}

TEST(boundsintersect_touching)
{
	/* touching at a face counts as intersecting (no strict separation) */
	vec3_t m1 = { 0, 0, 0 }, M1 = { 10, 10, 10 };
	vec3_t m2 = { 10, 0, 0 }, M2 = { 20, 10, 10 };
	OA_ASSERT(BoundsIntersect(m1, M1, m2, M2) == qtrue);
}

TEST(boundsintersectsphere_center_inside)
{
	vec3_t m = { 0, 0, 0 }, M = { 10, 10, 10 };
	vec3_t c = { 5, 5, 5 };
	OA_ASSERT(BoundsIntersectSphere(m, M, c, 1.0f) == qtrue);
}

TEST(boundsintersectsphere_far_away)
{
	vec3_t m = { 0, 0, 0 }, M = { 10, 10, 10 };
	vec3_t c = { 100, 100, 100 };
	OA_ASSERT(BoundsIntersectSphere(m, M, c, 1.0f) == qfalse);
}

TEST(boundsintersectpoint_inside)
{
	vec3_t m = { 0, 0, 0 }, M = { 10, 10, 10 };
	vec3_t p = { 5, 5, 5 };
	OA_ASSERT(BoundsIntersectPoint(m, M, p) == qtrue);
}

TEST(boundsintersectpoint_outside)
{
	vec3_t m = { 0, 0, 0 }, M = { 10, 10, 10 };
	vec3_t p = { 11, 5, 5 };
	OA_ASSERT(BoundsIntersectPoint(m, M, p) == qfalse);
}

TEST(radiusfrombounds_axis_aligned)
{
	vec3_t m = { -3, -4, -12 }, M = { 3, 4, 12 };
	/* corner = (3,4,12) -> length = 13 */
	OA_ASSERT_FLOAT(RadiusFromBounds(m, M), 13.0f, 1e-3f);
}

TEST(angle_normalize360_wraps)
{
	/* AngleNormalize360 uses fixed-point math, so allow a small tolerance */
	OA_ASSERT_FLOAT(AngleNormalize360(370.0f), 10.0f, 1e-2f);
	OA_ASSERT_FLOAT(AngleNormalize360(-10.0f), 350.0f, 1e-2f);
	OA_ASSERT_FLOAT(AngleNormalize360(360.0f), 0.0f, 1e-2f);
}

TEST(angle_normalize180_wraps)
{
	OA_ASSERT_FLOAT(AngleNormalize180(190.0f), -170.0f, 1e-2f);
	OA_ASSERT_FLOAT(AngleNormalize180(-190.0f), 170.0f, 1e-2f);
	OA_ASSERT_FLOAT(AngleNormalize180(90.0f), 90.0f, 1e-2f);
}

TEST(angle_delta)
{
	/* AngleDelta(a,b) = AngleNormalize180(a - b); fixed-point, tolerance */
	OA_ASSERT_FLOAT(AngleDelta(10.0f, 20.0f), -10.0f, 1e-2f);
	OA_ASSERT_FLOAT(AngleDelta(350.0f, 10.0f), -20.0f, 1e-2f);
}

TEST(lerp_angle)
{
	OA_ASSERT_FLOAT(LerpAngle(0.0f, 90.0f, 0.5f), 45.0f, 1e-3f);
}

TEST(dir_to_byte_roundtrip)
{
	/* every direction byte should map to a direction whose canonical byte
	 * encoding is stable: DirToByte(ByteToDir(b)) == b for reachable bytes */
	vec3_t dir;
	for (int b = 0; b < 256; b++) {
		ByteToDir(b, dir);
		int b2 = DirToByte(dir);
		/* re-encoding the decoded direction must be stable */
		ByteToDir(b2, dir);
		OA_ASSERT_INT(DirToByte(dir), b2);
	}
}

TEST(q_log2)
{
	OA_ASSERT_INT(Q_log2(1), 0);
	OA_ASSERT_INT(Q_log2(2), 1);
	OA_ASSERT_INT(Q_log2(8), 3);
	OA_ASSERT_INT(Q_log2(256), 8);
}

TEST(q_isnan)
{
	/* INF is not NaN */
	OA_ASSERT(Q_isnan(1.0f / 0.0f) == qfalse);
	/* actual NaN */
	OA_ASSERT(Q_isnan(0.0f / 0.0f) == qtrue);
	OA_ASSERT(Q_isnan(42.0f) == qfalse);
}
