# OpenArena Unit Tests

Minimal, dependency-free unit test harness for the OpenArena engine. No
external test framework (CUnit/Check/gtest) required — just gcc + libm.

## What is covered

These tests exercise the engine's `qcommon` string and token-parser helpers
(`code/qcommon/q_shared.c`), which sit on the hot path for shader and config
loading. They compile and run **without SDL2 or OpenGL**, so they work in any
CI environment.

| Suite                 | Covers                                                    |
|-----------------------|-----------------------------------------------------------|
| `tests_qshared.c`     | `Q_strncpyz`, `Q_stricmpn`, `Com_sprintf`, `COM_StripExtension`, `COM_SkipPath`, `COM_Compress`, `Q_CountChar`, `Q_strlwr/upr`, `Q_strcat` |
| `tests_qparse.c`      | `COM_Parse` / `COM_ParseExt` token parsing, quoting, comments |
| `tests_qmath.c`       | vec3 macros (`VectorAdd`/`Subtract`/`Clear`/`Set`/`Copy`/`DotProduct`), `VectorNormalize`, `CrossProduct`, `AngleVectors`, `ProjectPointOnPlane`, `RotatePointAroundVector`, `Q_rsqrt`, `MakeNormalVectors` |
| `tests_qinfo.c`      | `Info_ValueForKey`, `Info_SetValueForKey` (+`_Big`), `Info_RemoveKey` (+`_Big`), `Info_Validate`, `Info_NextPair` |
| `tests_qinfo_overflow.c` | buffer-overflow safety for `Info_SetValueForKey` (separate `test_overflow` binary with non-aborting error stub + canary) |

## Run locally

From the repo root:

    make -f tests/Makefile run

This builds and runs two binaries:

* `tests/test_all` — the main suite (strings, parser, vector math, info strings).
* `tests/test_overflow` — buffer-overflow safety tests for the info-string
  setters. These use a non-aborting `Com_Error` stub (`test_stubs_nofail.c`)
  and a canary placed after the buffer, so an overrun is detected instead of
  crashing the test process.

Or build manually (example for the main suite):

    cd tests
    gcc -I../code/qcommon -I../code -o test_all \
        oa_test_run.c test_main.c test_stubs.c \
        tests_qshared.c tests_qparse.c tests_qmath.c tests_qinfo.c \
        ../code/qcommon/q_shared.c ../code/qcommon/q_math.c -lm
    ./test_all

The binaries exit non-zero if any test fails, so they are CI-friendly.

## How it works

* `oa_test.h` — assertion macros (`OA_ASSERT`, `OA_ASSERT_INT`,
  `OA_ASSERT_STR`, `OA_ASSERT_STRN`, `OA_ASSERT_FLOAT`, `OA_ASSERT_VEC3`,
  `OA_ASSERT_VEC3_ZERO`) plus the `TEST(name)` macro. Each `TEST` self-registers
  via a GCC/Clang constructor attribute (no linker-section tricks, no
  global-state-in-header pitfalls).
* `oa_test_run.c` — the runner and registry. `test_main.c` calls `oa_test_run()`.
* `test_stubs.c` — tiny stubs for engine symbols (`Com_Error`, `Com_Printf`, …)
  referenced by `q_shared.c`, so the helpers can be tested standalone.

Float/vector assertions take an explicit tolerance (`eps`) because vector math
uses `float` and is not bit-identical across platforms.

## Adding a test

1. Pick or create a `tests_*.c` file.
2. Write `TEST(my_check) { ... OA_ASSERT_*(...); }`.
3. Add the file to `SUITES` in `tests/Makefile`.

## Known limitations (documented, not bugs)

* `COM_SkipPath` only recognises `/` as a separator; backslash paths are
  normalised elsewhere in the engine. The test asserts the actual behaviour so
  a regression in `/` handling is caught.
* `COM_Compress` keeps one newline between remaining tokens after stripping a
  `//` comment line.
