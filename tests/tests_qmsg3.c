/*
 * tests_qmsg3.c - More qcommon/msg.c coverage: out-of-band messages (raw,
 * non-Huffman), MSG_Copy lifecycle, and the key-based delta codec
 * (MSG_WriteDeltaKey / MSG_ReadDeltaKey). These need only q_shared.c +
 * huffman.c and exercise paths not covered by the main msg suite.
 *
 * Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I.. -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c tests_qmath.c tests_qinfo.c \
 *       tests_qstr.c tests_qmatrix.c tests_qtoken.c tests_qchar.c \
 *       tests_qhuffman.c tests_qmsg.c tests_qformat.c tests_qmath2.c \
 *       tests_qmath3.c tests_qstring2.c tests_qmsg2.c tests_qmsg3.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/huffman.c ../code/qcommon/msg.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"
#include "qcommon.h"

/* key-based delta helpers are local to msg.c; declare for the test */
void MSG_WriteDeltaKey(msg_t *msg, int key, int oldV, int newV, int bits);
int  MSG_ReadDeltaKey(msg_t *msg, int key, int oldV, int bits);

#define MSG3_BUF 4096

static msg_t make_oob(byte *data, int len)
{
	msg_t m;
	MSG_InitOOB(&m, data, len);
	return m;
}

TEST(msg_oob_write_read_byte)
{
	/* OOB messages are not Huffman-compressed; a byte written is read back
	 * as the same raw byte. */
	byte data[MSG3_BUF];
	msg_t m = make_oob(data, MSG3_BUF);
	MSG_WriteByte(&m, 0xAB);
	MSG_WriteByte(&m, 0x42);
	MSG_BeginReadingOOB(&m);
	OA_ASSERT_INT(MSG_ReadByte(&m), 0xAB);
	OA_ASSERT_INT(MSG_ReadByte(&m), 0x42);
}

TEST(msg_oob_write_read_long)
{
	byte data[MSG3_BUF];
	msg_t m = make_oob(data, MSG3_BUF);
	MSG_WriteLong(&m, 0x12345678);
	MSG_BeginReadingOOB(&m);
	OA_ASSERT_INT(MSG_ReadLong(&m), 0x12345678);
}

TEST(msg_copy_preserves_contents)
{
	byte src_data[MSG3_BUF], dst_data[MSG3_BUF];
	msg_t m;
	MSG_Init(&m, src_data, MSG3_BUF);
	MSG_WriteLong(&m, 0x12345678);
	MSG_WriteByte(&m, 77);

	msg_t dst;
	MSG_Copy(&dst, dst_data, MSG3_BUF, &m);
	/* the copy must report the same cursize and read back the same bytes */
	OA_ASSERT_INT(dst.cursize, m.cursize);
	MSG_BeginReading(&dst);
	OA_ASSERT_INT(MSG_ReadLong(&dst), 0x12345678);
	OA_ASSERT_INT(MSG_ReadByte(&dst), 77);
}

TEST(msg_delta_key_unchanged)
{
	/* delta-key helpers write a 1-bit changed-flag, which requires the
	 * Huffman (bitstream) path, not OOB. Use MSG_Init (oob = qfalse). */
	byte data[MSG3_BUF];
	msg_t m;
	MSG_Init(&m, data, MSG3_BUF);
	/* key must be < 2^bits so the read-side XOR (key & kbitmask[bits-1])
	 * cancels the write-side XOR exactly. */
	MSG_WriteDeltaKey(&m, 0x123, 100, 100, 12);
	MSG_BeginReading(&m);
	OA_ASSERT_INT(MSG_ReadDeltaKey(&m, 0x123, 100, 12), 100);
}

TEST(msg_delta_key_changed)
{
	byte data[MSG3_BUF];
	msg_t m;
	MSG_Init(&m, data, MSG3_BUF);
	MSG_WriteDeltaKey(&m, 0x123, 100, 250, 12);
	MSG_BeginReading(&m);
	OA_ASSERT_INT(MSG_ReadDeltaKey(&m, 0x123, 100, 12), 250);
}
