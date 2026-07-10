/*
 * tests_qhuffman.c - Round-trip tests for the adaptive Huffman codec in
 * qcommon/huffman.c using the engine's real network-path API
 * (Huff_Compress / Huff_Decompress on a msg_t buffer).
 *
 * Huff_Compress/Huff_Decompress are the functions actually used by the
 * engine's message layer. They grow the compressor and decompressor trees
 * in lock-step internally, so a compress-then-decompress on the same buffer
 * must reproduce the original bytes exactly. This is a genuine round-trip
 * test of the wire codec.
 *
 * Build & run (combined):
 *   cd engine/tests
 *   gcc -I../code/qcommon -I../code -o test_all \
 *       oa_test_run.c test_main.c test_stubs.c \
 *       tests_qshared.c tests_qparse.c tests_qmath.c tests_qinfo.c \
 *       tests_qstr.c tests_qmatrix.c tests_qtoken.c tests_qchar.c tests_qhuffman.c \
 *       ../code/qcommon/q_shared.c ../code/qcommon/q_math.c \
 *       ../code/qcommon/huffman.c -lm
 *   ./test_all
 */
#include "oa_test.h"

#include "q_shared.h"
#include "qcommon.h"

#define HUFF_BUF 65536

/* Compress `len` bytes from `in` into a msg_t, then decompress back into
 * `dec`, and return qtrue iff dec == in. Huff_Init seeds fresh trees. */
static qboolean huff_roundtrip(const byte *in, int len, byte *dec)
{
	msg_t mbuf;
	static byte data[HUFF_BUF];

	Com_Memset(&mbuf, 0, sizeof(mbuf));
	mbuf.data = data;
	mbuf.maxsize = HUFF_BUF;
	mbuf.cursize = len;
	Com_Memcpy(mbuf.data, in, len);

	Huff_Compress(&mbuf, 0);
	Huff_Decompress(&mbuf, 0);

	if (mbuf.cursize != len)
		return qfalse;
	for (int i = 0; i < len; i++) {
		if (mbuf.data[i] != in[i])
			return qfalse;
	}
	(void)dec;
	return qtrue;
}

TEST(huff_roundtrip_single_byte)
{
	byte in[] = { 'A' };
	byte dec[1];
	OA_ASSERT(huff_roundtrip(in, 1, dec) == qtrue);
}

TEST(huff_roundtrip_simple_string)
{
	byte in[] = "hello";
	byte dec[8];
	OA_ASSERT(huff_roundtrip(in, 5, dec) == qtrue);
}

TEST(huff_roundtrip_repeated_symbols)
{
	byte in[] = "aaaaaaaaaa";
	byte dec[16];
	OA_ASSERT(huff_roundtrip(in, 10, dec) == qtrue);
}

TEST(huff_roundtrip_all_bytes)
{
	byte in[256];
	byte dec[256];
	for (int i = 0; i < 256; i++)
		in[i] = (byte)i;
	OA_ASSERT(huff_roundtrip(in, 256, dec) == qtrue);
}

TEST(huff_roundtrip_mixed_binary)
{
	byte in[] = { 0x00, 0xFF, 0x10, 0x42, 0x00, 0x7E, 0x80 };
	byte dec[8];
	OA_ASSERT(huff_roundtrip(in, 7, dec) == qtrue);
}

TEST(huff_roundtrip_long_repetitive)
{
	/* a longer, repetitive payload exercises the adaptive tree growth */
	byte in[300];
	byte dec[300];
	for (int i = 0; i < 300; i++)
		in[i] = (byte)("the quick brown fox jumps over the lazy dog"[i % 44]);
	OA_ASSERT(huff_roundtrip(in, 300, dec) == qtrue);
}
