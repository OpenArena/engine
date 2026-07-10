/*
 * oa_test_run.c - Test runner for the OpenArena minimal test harness.
 *
 * Tests self-register via constructors (see oa_test.h). This file provides the
 * registry and the run loop.
 */
#include "oa_test.h"

static const char *oa_test_names[OA_TEST_MAX];
static oa_test_fn   oa_test_fns[OA_TEST_MAX];
static int         oa_test_n = 0;

int oa_test_failed_count = 0;
const char *oa_test_current = "";

void oa_test_register(const char *name, oa_test_fn fn)
{
	if (oa_test_n >= OA_TEST_MAX)
		return;
	oa_test_names[oa_test_n] = name;
	oa_test_fns[oa_test_n] = fn;
	oa_test_n++;
}

int oa_test_run(void)
{
	int total = 0;
	int failed_cases = 0;

	for (int i = 0; i < oa_test_n; i++) {
		oa_test_current = oa_test_names[i];
		int before = oa_test_failed_count;
		oa_test_fns[i]();
		total++;
		if (oa_test_failed_count > before) {
			failed_cases++;
		} else {
			printf("ok   - %s\n", oa_test_names[i]);
		}
	}

	printf("\n%d tests, %d failed\n", total, failed_cases);
	return failed_cases == 0 ? 0 : 1;
}
