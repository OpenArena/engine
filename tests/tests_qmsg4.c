/*
 * tests_qmsg4.c - Float key-delta coverage for qcommon/msg.c:
 * MSG_WriteDeltaKeyFloat / MSG_ReadDeltaKeyFloat. These are local to msg.c
 * and need no entityState_t; they round-trip a float with a key XOR salt.
 *
 * Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I.. -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c tests_qmath.c tests_qinfo.c \
 *       tests_qstr.c tests_qmatrix.c tests_qtoken.c tests_qchar.c \
 *       tests_qhuffman.c tests_qmsg.c tests_qformat.c tests_qmath2.c \
 *       tests_qmath3.c tests_qstring2.c tests_qmsg2.c tests_qmsg3.c \
 *       tests_qmath4.c tests_qmsg4.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/huffman.c ../code/qcommon/msg.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"
#include "qcommon.h"

/* local to msg.c; declare for the test */
void MSG_WriteDeltaKeyFloat(msg_t *msg, int key, float oldV, float newV);
float MSG_ReadDeltaKeyFloat(msg_t *msg, int key, float oldV);

#define MSG4_BUF 4096

TEST(msg_delta_key_float_unchanged)
{
	/* unchanged float -> reads back oldV. Needs Huffman (bitstream) path. */
	byte data[MSG4_BUF];
	msg_t m;
	MSG_Init(&m, data, MSG4_BUF);
	MSG_WriteDeltaKeyFloat(&m, 0xCAFEBABE, 1.5f, 1.5f);
	MSG_BeginReading(&m);
	OA_ASSERT_FLOAT(MSG_ReadDeltaKeyFloat(&m, 0xCAFEBABE, 1.5f), 1.5f, 1e-6f);
}

TEST(msg_delta_key_float_changed)
{
	byte data[MSG4_BUF];
	msg_t m;
	MSG_Init(&m, data, MSG4_BUF);
	MSG_WriteDeltaKeyFloat(&m, 0xCAFEBABE, 1.0f, 2.25f);
	MSG_BeginReading(&m);
	OA_ASSERT_FLOAT(MSG_ReadDeltaKeyFloat(&m, 0xCAFEBABE, 1.0f), 2.25f, 1e-6f);
}

TEST(msg_delta_key_float_negative)
{
	byte data[MSG4_BUF];
	msg_t m;
	MSG_Init(&m, data, MSG4_BUF);
	MSG_WriteDeltaKeyFloat(&m, 0x12345678, -3.75f, 4.125f);
	MSG_BeginReading(&m);
	OA_ASSERT_FLOAT(MSG_ReadDeltaKeyFloat(&m, 0x12345678, -3.75f), 4.125f, 1e-6f);
}
