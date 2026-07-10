/*
 * oa_test.h - Minimal, dependency-free unit test harness for OpenArena.
 *
 * No external test framework required (no CUnit/Check/gtest). Just compile
 * a tests_*.c file together with this header and the engine source files it
 * exercises, link, and run the resulting binary.
 *
 * Usage:
 *   TEST(name) { ... assertions ... }
 *   int main(void) { return oa_test_run(); }
 *
 * Registration uses a GCC/Clang constructor attribute (portable, no linker
 * section tricks). Each TEST() registers itself before main() runs.
 *
 * Assertions:
 *   OA_ASSERT(cond)                 - expect cond != 0
 *   OA_ASSERT_INT(a, b)             - expect (long)a == (long)b
 *   OA_ASSERT_STR(a, b)             - expect strcmp(a,b)==0 (NULL-safe)
 *   OA_ASSERT_STRN(a, b, n)         - expect strncmp(a,b,n)==0
 */
#ifndef OA_TEST_H
#define OA_TEST_H

#include <stdio.h>
#include <string.h>
#include <stdarg.h>

typedef void (*oa_test_fn)(void);

#define OA_TEST_MAX 1024

void oa_test_register(const char *name, oa_test_fn fn);

#define OA_TEST_CONCAT2(a, b) a##b
#define OA_TEST_CONCAT(a, b)  OA_TEST_CONCAT2(a, b)

#define TEST(name)                                                       \
	static void oa_test_func_##name(void);                              \
	static void __attribute__((constructor))                            \
		OA_TEST_CONCAT(oa_test_ctor_, name)(void) {                 \
		oa_test_register(#name, oa_test_func_##name);            \
	}                                                                \
	static void oa_test_func_##name(void)

/* Shared state lives in oa_test_run.c (defined there, declared extern here)
 * so that every translation unit references the SAME counters. Declaring
 * these `static` in a header would give each .c file its own copy and the
 * runner would never see the failures reported by the tests. */
extern int oa_test_failed_count;
extern const char *oa_test_current;

#define OA_FAIL(fmt, ...)                                               \
	do {                                                            \
		oa_test_failed_count++;                                 \
		fprintf(stderr, "  FAIL [%s] %s:%d: " fmt "\n",         \
			oa_test_current, __FILE__, __LINE__, ##__VA_ARGS__); \
	} while (0)

#define OA_ASSERT(cond)                                                 \
	do {                                                            \
		if (!(cond)) { OA_FAIL("OA_ASSERT(%s)", #cond); }      \
	} while (0)

#define OA_ASSERT_INT(a, b)                                             \
	do {                                                            \
		long _a = (long)(a), _b = (long)(b);                   \
		if (_a != _b) {                                      \
			OA_FAIL("OA_ASSERT_INT(%s,%s) -> %ld != %ld", \
				#a, #b, _a, _b);                        \
		}                                                 \
	} while (0)

#define OA_ASSERT_STR(a, b)                                             \
	do {                                                            \
		const char *_a = (a), *_b = (b);                        \
		int _eq = (_a == _b) ? 1 :                             \
			(_a && _b) ? (strcmp(_a, _b) == 0) : 0;        \
		if (!_eq) {                                          \
			OA_FAIL("OA_ASSERT_STR(%s,%s) -> \"%s\" != \"%s\"", \
				#a, #b, _a ? _a : "(null)",             \
				_b ? _b : "(null)");                    \
		}                                                 \
	} while (0)

#define OA_ASSERT_STRN(a, b, n)                                         \
	do {                                                            \
		const char *_a = (a), *_b = (b);                        \
		if (strncmp(_a, _b, (size_t)(n)) != 0) {               \
			OA_FAIL("OA_ASSERT_STRN(%s,%s,%d)", #a, #b,    \
				(int)(n));                              \
		}                                                 \
	} while (0)

/* Floating-point assertions with relative/absolute tolerance. Useful for
 * vector math where exact equality is not guaranteed across platforms. */
#define OA_ASSERT_FLOAT(a, b, eps)                                      \
	do {                                                            \
		double _a = (double)(a), _b = (double)(b);              \
		double _d = (_a > _b) ? (_a - _b) : (_b - _a);         \
		if (_d > (double)(eps)) {                              \
			OA_FAIL("OA_ASSERT_FLOAT(%s,%s,%s) -> %g vs %g " \
				"(diff %g)", #a, #b, #eps, _a, _b, _d);   \
		}                                                 \
	} while (0)

/* Component-wise vec3 assertion within tolerance eps. */
#define OA_ASSERT_VEC3(a, b, eps)                                       \
	do {                                                            \
		OA_ASSERT_FLOAT((a)[0], (b)[0], eps);                  \
		OA_ASSERT_FLOAT((a)[1], (b)[1], eps);                  \
		OA_ASSERT_FLOAT((a)[2], (b)[2], eps);                  \
	} while (0)

#define OA_ASSERT_VEC3_ZERO(a, eps)                                     \
	do {                                                            \
		OA_ASSERT_FLOAT((a)[0], 0.0, eps);                     \
		OA_ASSERT_FLOAT((a)[1], 0.0, eps);                     \
		OA_ASSERT_FLOAT((a)[2], 0.0, eps);                     \
	} while (0)

/* Provided by oa_test_run.c */
int oa_test_run(void);

#endif /* OA_TEST_H */
