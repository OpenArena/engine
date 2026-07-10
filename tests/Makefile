# Makefile for OpenArena standalone unit tests.
#
# These tests exercise the engine's qcommon string/parse/vector/info helpers
# WITHOUT requiring SDL2 or OpenGL, so they run anywhere gcc + libm are
# available (including CI). They use the minimal harness in oa_test.h /
# oa_test_run.c.
#
# Usage:
#   make -f tests/Makefile
#   ./tests/test_all
#   ./tests/test_overflow      # buffer-overflow safety tests
#
# Add more suites by appending a tests_*.c file and listing it in TEST_SRCS.

CC      ?= gcc
CFLAGS  ?= -O2 -Wall -Wextra
INC     := -Icode/qcommon -Icode
LIBS    := -lm

TEST_DIR := tests
ENGINE   := code/qcommon
STUBS    := $(TEST_DIR)/test_stubs.c $(TEST_DIR)/test_main.c $(TEST_DIR)/oa_test_run.c
SUITES   := $(TEST_DIR)/tests_qshared.c $(TEST_DIR)/tests_qparse.c \
            $(TEST_DIR)/tests_qmath.c $(TEST_DIR)/tests_qinfo.c \
            $(TEST_DIR)/tests_qstr.c
ENG_SRC  := $(ENGINE)/q_shared.c $(ENGINE)/q_math.c

TARGET      := $(TEST_DIR)/test_all
TARGET_OVER := $(TEST_DIR)/test_overflow

.PHONY: all run clean

all: $(TARGET) $(TARGET_OVER)

$(TARGET): $(STUBS) $(SUITES) $(ENG_SRC)
	$(CC) $(CFLAGS) $(INC) -o $@ $(STUBS) $(SUITES) $(ENG_SRC) $(LIBS)

# Overflow-safety tests use a non-aborting Com_Error stub so the overrun guard
# can be observed without killing the test process.
$(TARGET_OVER): $(TEST_DIR)/test_stubs_nofail.c $(TEST_DIR)/test_main.c \
                $(TEST_DIR)/oa_test_run.c $(TEST_DIR)/tests_qinfo_overflow.c $(ENG_SRC)
	$(CC) $(CFLAGS) $(INC) -o $@ $(TEST_DIR)/test_stubs_nofail.c \
	    $(TEST_DIR)/test_main.c $(TEST_DIR)/oa_test_run.c \
	    $(TEST_DIR)/tests_qinfo_overflow.c $(ENG_SRC) $(LIBS)

run: all
	./$(TARGET)
	./$(TARGET_OVER)

clean:
	rm -f $(TARGET) $(TARGET_OVER)
