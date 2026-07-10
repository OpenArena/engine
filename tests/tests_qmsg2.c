/*
 * tests_qmsg2.c - More round-trip tests for qcommon/msg.c: raw data,
 * lookahead, big strings, 16-bit angles, and the shared-bits delta codec
 * (MSG_WriteDelta / MSG_ReadDelta / MSG_WriteDeltaFloat / MSG_ReadDeltaFloat).
 *
 * These operate on a plain msg_t and need only q_shared.c + huffman.c.
 *
 * Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I.. -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c tests_qmath.c tests_qinfo.c \
 *       tests_qstr.c tests_qmatrix.c tests_qtoken.c tests_qchar.c \
 *       tests_qhuffman.c tests_qmsg.c tests_qformat.c tests_qmath2.c \
 *       tests_qmath3.c tests_qstring2.c tests_qmsg2.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/huffman.c ../code/qcommon/msg.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"
#include "qcommon.h"

/* These delta helpers are defined in msg.c but not exported in qcommon.h,
 * so declare them locally for the test. */
void MSG_WriteDelta(msg_t *msg, int oldV, int newV, int bits);
int  MSG_ReadDelta(msg_t *msg, int oldV, int bits);
void MSG_WriteDeltaFloat(msg_t *msg, float oldV, float newV);
float MSG_ReadDeltaFloat(msg_t *msg, float oldV);

#define MSG2_BUF 4096

static msg_t make_msg2(byte *data, int len)
{
	msg_t m;
	MSG_Init(&m, data, len);
	return m;
}

TEST(msg_write_read_data)
{
	byte data[MSG2_BUF];
	byte payload[8] = { 0x00, 0xFF, 0x10, 0x42, 0x99, 0xAB, 0xCD, 0xEF };
	msg_t m = make_msg2(data, MSG2_BUF);
	MSG_WriteData(&m, payload, 8);
	MSG_BeginReading(&m);
	byte out[8];
	MSG_ReadData(&m, out, 8);
	for (int i = 0; i < 8; i++)
		OA_ASSERT_INT(out[i], payload[i]);
}

TEST(msg_lookahead_byte_does_not_advance)
{
	byte data[MSG2_BUF];
	msg_t m = make_msg2(data, MSG2_BUF);
	MSG_WriteByte(&m, 42);
	MSG_WriteByte(&m, 99);
	MSG_BeginReading(&m);
	/* lookahead returns the next byte but does not consume it */
	OA_ASSERT_INT(MSG_LookaheadByte(&m), 42);
	OA_ASSERT_INT(MSG_LookaheadByte(&m), 42); /* still 42, pointer unchanged */
	/* now actually read it */
	OA_ASSERT_INT(MSG_ReadByte(&m), 42);
	OA_ASSERT_INT(MSG_ReadByte(&m), 99);
}

TEST(msg_write_read_big_string)
{
	/* a string longer than the normal 1024-char MSG_ReadString limit path */
	byte data[MSG2_BUF];
	char longstr[2000];
	for (int i = 0; i < 1999; i++)
		longstr[i] = (char)('a' + (i % 26));
	longstr[1999] = 0;
	msg_t m = make_msg2(data, MSG2_BUF);
	MSG_WriteBigString(&m, longstr);
	MSG_BeginReading(&m);
	char *back = MSG_ReadBigString(&m);
	OA_ASSERT_STR(back, longstr);
}

TEST(msg_write_read_angle16)
{
	byte data[MSG2_BUF];
	msg_t m = make_msg2(data, MSG2_BUF);
	/* 16-bit angle has ~0.0055 degree precision */
	MSG_WriteAngle16(&m, 123.456f);
	MSG_BeginReading(&m);
	float a = MSG_ReadAngle16(&m);
	OA_ASSERT_FLOAT(a, 123.456f, 0.01f);
}

TEST(msg_delta_unchanged)
{
	/* when newV == oldV, the delta encodes "no change" and reads back oldV */
	byte data[MSG2_BUF];
	msg_t m = make_msg2(data, MSG2_BUF);
	MSG_WriteDelta(&m, 100, 100, 12);
	MSG_BeginReading(&m);
	OA_ASSERT_INT(MSG_ReadDelta(&m, 100, 12), 100);
}

TEST(msg_delta_changed)
{
	byte data[MSG2_BUF];
	msg_t m = make_msg2(data, MSG2_BUF);
	MSG_WriteDelta(&m, 100, 250, 12);
	MSG_BeginReading(&m);
	OA_ASSERT_INT(MSG_ReadDelta(&m, 100, 12), 250);
}

TEST(msg_delta_float_unchanged)
{
	byte data[MSG2_BUF];
	msg_t m = make_msg2(data, MSG2_BUF);
	MSG_WriteDeltaFloat(&m, 3.5f, 3.5f);
	MSG_BeginReading(&m);
	OA_ASSERT_FLOAT(MSG_ReadDeltaFloat(&m, 3.5f), 3.5f, 1e-6f);
}

TEST(msg_delta_float_changed)
{
	byte data[MSG2_BUF];
	msg_t m = make_msg2(data, MSG2_BUF);
	MSG_WriteDeltaFloat(&m, 1.0f, 2.0f);
	MSG_BeginReading(&m);
	OA_ASSERT_FLOAT(MSG_ReadDeltaFloat(&m, 1.0f), 2.0f, 1e-6f);
}
