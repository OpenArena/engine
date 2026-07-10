/*
 * test_main.c - Entry point for the OpenArena unit test suites.
 *
 * Links against oa_test_run.c (the runner) and any number of tests_*.c files.
 */
#include "oa_test.h"

int main(void)
{
	return oa_test_run();
}
