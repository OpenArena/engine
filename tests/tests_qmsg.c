/*
 * tests_qmsg.c - Round-trip tests for the network message buffer in
 * qcommon/msg.c. The MSG_Write / MSG_Read family operates on a msg_t backed
 * by a plain byte array, so they link with only q_shared.c (no memory
 * manager, no SDL).
 *
 * Each test writes a value (or several) into a msg_t, seeks back to the
 * start with MSG_BeginReading, and reads it back, asserting exact identity.
 * This is a genuine serialize/deserialize test of the wire format.
 *
 * Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c tests_qmath.c tests_qinfo.c \
 *       tests_qstr.c tests_qmatrix.c tests_qtoken.c tests_qchar.c \
 *       tests_qhuffman.c tests_qmsg.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/huffman.c ../code/qcommon/msg.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"
#include "qcommon.h"

#define MSG_BUF 4096

static msg_t make_msg(byte *data, int len)
{
	msg_t m;
	MSG_Init(&m, data, len);
	return m;
}

TEST(msg_init_clears_buffer)
{
	byte data[MSG_BUF] = { 0 };
	msg_t m = make_msg(data, MSG_BUF);
	OA_ASSERT_INT(m.cursize, 0);
	OA_ASSERT_INT(m.maxsize, MSG_BUF);
	OA_ASSERT(m.data == data);
}

TEST(msg_write_read_char)
{
	byte data[MSG_BUF];
	msg_t m = make_msg(data, MSG_BUF);
	MSG_WriteChar(&m, 'A');
	MSG_WriteChar(&m, -5);
	MSG_BeginReading(&m);
	OA_ASSERT_INT(MSG_ReadChar(&m), 'A');
	OA_ASSERT_INT(MSG_ReadChar(&m), -5);
}

TEST(msg_write_read_byte)
{
	byte data[MSG_BUF];
	msg_t m = make_msg(data, MSG_BUF);
	MSG_WriteByte(&m, 0);
	MSG_WriteByte(&m, 255);
	MSG_WriteByte(&m, 128);
	MSG_BeginReading(&m);
	OA_ASSERT_INT(MSG_ReadByte(&m), 0);
	OA_ASSERT_INT(MSG_ReadByte(&m), 255);
	OA_ASSERT_INT(MSG_ReadByte(&m), 128);
}

TEST(msg_write_read_short)
{
	byte data[MSG_BUF];
	msg_t m = make_msg(data, MSG_BUF);
	MSG_WriteShort(&m, -12345);
	MSG_WriteShort(&m, 32767);
	MSG_WriteShort(&m, 0);
	MSG_BeginReading(&m);
	OA_ASSERT_INT(MSG_ReadShort(&m), -12345);
	OA_ASSERT_INT(MSG_ReadShort(&m), 32767);
	OA_ASSERT_INT(MSG_ReadShort(&m), 0);
}

TEST(msg_write_read_long)
{
	byte data[MSG_BUF];
	msg_t m = make_msg(data, MSG_BUF);
	MSG_WriteLong(&m, 0x12345678);
	MSG_WriteLong(&m, -2147483647);
	MSG_WriteLong(&m, 123456789);
	MSG_BeginReading(&m);
	OA_ASSERT_INT(MSG_ReadLong(&m), 0x12345678);
	OA_ASSERT_INT(MSG_ReadLong(&m), -2147483647);
	OA_ASSERT_INT(MSG_ReadLong(&m), 123456789);
}

TEST(msg_write_read_float)
{
	byte data[MSG_BUF];
	msg_t m = make_msg(data, MSG_BUF);
	MSG_WriteFloat(&m, 3.14159f);
	MSG_WriteFloat(&m, -2.5f);
	MSG_WriteFloat(&m, 0.0f);
	MSG_BeginReading(&m);
	OA_ASSERT_FLOAT(MSG_ReadFloat(&m), 3.14159f, 1e-4f);
	OA_ASSERT_FLOAT(MSG_ReadFloat(&m), -2.5f, 1e-6f);
	OA_ASSERT_FLOAT(MSG_ReadFloat(&m), 0.0f, 1e-9f);
}

TEST(msg_write_read_string)
{
	byte data[MSG_BUF];
	msg_t m = make_msg(data, MSG_BUF);
	MSG_WriteString(&m, "hello world");
	MSG_WriteString(&m, "");
	MSG_WriteString(&m, "trailing");
	MSG_BeginReading(&m);
	OA_ASSERT_STR(MSG_ReadString(&m), "hello world");
	OA_ASSERT_STR(MSG_ReadString(&m), "");
	OA_ASSERT_STR(MSG_ReadString(&m), "trailing");
}

TEST(msg_write_read_bits)
{
	/* write individual bits and read them back */
	byte data[MSG_BUF];
	msg_t m = make_msg(data, MSG_BUF);
	int vals[] = { 1, 0, 1, 1, 0, 1, 0, 0, 1, 0, 1, 1 };
	for (int i = 0; i < 12; i++)
		MSG_WriteBits(&m, vals[i], 1);
	MSG_BeginReading(&m);
	for (int i = 0; i < 12; i++)
		OA_ASSERT_INT(MSG_ReadBits(&m, 1), vals[i]);
}

TEST(msg_write_read_multibit)
{
	/* write a 12-bit value and read it back */
	byte data[MSG_BUF];
	msg_t m = make_msg(data, MSG_BUF);
	MSG_WriteBits(&m, 0xABC, 12);
	MSG_BeginReading(&m);
	OA_ASSERT_INT(MSG_ReadBits(&m, 12), 0xABC);
}

TEST(msg_hashkey_deterministic)
{
	/* same input -> same hash; different input -> (very likely) different */
	OA_ASSERT_INT(MSG_HashKey("connect", 32), MSG_HashKey("connect", 32));
	OA_ASSERT_INT(MSG_HashKey("connect", 32), MSG_HashKey("connect", 32));
	/* not required to differ, but sanity: a longer string hashes too */
	OA_ASSERT_INT(MSG_HashKey("disconnect", 32), MSG_HashKey("disconnect", 32));
}

TEST(msg_clear_resets_cursize)
{
	byte data[MSG_BUF];
	msg_t m = make_msg(data, MSG_BUF);
	MSG_WriteLong(&m, 42);
	OA_ASSERT(m.cursize > 0);
	MSG_Clear(&m);
	OA_ASSERT_INT(m.cursize, 0);
	/* after clear we can write again from the start */
	MSG_WriteLong(&m, 99);
	MSG_BeginReading(&m);
	OA_ASSERT_INT(MSG_ReadLong(&m), 99);
}
