//================================================================================
// m1v_dec_eval - VLC テーブル
// $Id$
//================================================================================

#include "vlc.h"

const VLC_ENTRY vlc_table_b1[] =
{
	{ 0b1,           1,  1  },
	{ 0b011,         3,  2  },
	{ 0b010,         3,  3  },
	{ 0b0011,        4,  4  },
	{ 0b0010,        4,  5  },
	{ 0b00011,       5,  6  },
	{ 0b00010,       5,  7  },
	{ 0b0000111,     7,  8  },
	{ 0b0000110,     7,  9  },
	{ 0b00001011,    8,  10 },
	{ 0b00001010,    8,  11 },
	{ 0b00001001,    8,  12 },
	{ 0b00001000,    8,  13 },
	{ 0b00000111,    8,  14 },
	{ 0b00000110,    8,  15 },
	{ 0b0000010111,  10, 16 },
	{ 0b0000010110,  10, 17 },
	{ 0b0000010101,  10, 18 },
	{ 0b0000010100,  10, 19 },
	{ 0b0000010011,  10, 20 },
	{ 0b0000010010,  10, 21 },
	{ 0b00000100011, 11, 22 },
	{ 0b00000100010, 11, 23 },
	{ 0b00000100001, 11, 24 },
	{ 0b00000100000, 11, 25 },
	{ 0b00000011111, 11, 26 },
	{ 0b00000011110, 11, 27 },
	{ 0b00000011101, 11, 28 },
	{ 0b00000011100, 11, 29 },
	{ 0b00000011011, 11, 30 },
	{ 0b00000011010, 11, 31 },
	{ 0b00000011001, 11, 32 },
	{ 0b00000011000, 11, 33 },
	{ 0b00000001111, 11, 34 },
	{ 0b00000001000, 11, 35 },
	{ 0, 0, 0 },
};

const VLC_ENTRY vlc_table_b2a[] =
{
	{ 0b1,  1, 0b00001},
	{ 0b01, 2, 0b10001},
	{ 0, 0, 0 },
};

const VLC_ENTRY vlc_table_b2b[] =
{
	{ 0b1,      1, 0b01010 },
	{ 0b01,     2, 0b00010 },
	{ 0b001,    3, 0b01000 },
	{ 0b00011,  5, 0b00001 },
	{ 0b00010,  5, 0b11010 },
	{ 0b00001,  5, 0b10010 },
	{ 0b000001, 6, 0b10001 },
	{ 0, 0, 0 },
};

const VLC_ENTRY vlc_table_b2c[] =
{
	{ 0b10,     2, 0b01100 },
	{ 0b11,     2, 0b01110 },
	{ 0b010,    3, 0b00100 },
	{ 0b011,    3, 0b00110 },
	{ 0b0010,   4, 0b01000 },
	{ 0b0011,   4, 0b01010 },
	{ 0b00011,  5, 0b00001 },
	{ 0b00010,  5, 0b11110 },
	{ 0b000011, 6, 0b11010 },
	{ 0b000010, 6, 0b10110 },
	{ 0b000001, 6, 0b10001 },
	{ 0, 0, 0 },
};

const VLC_ENTRY vlc_table_b2d[] =
{
	{ 0b1, 1, 0b00001 },
	{ 0, 0, 0 },
};

const VLC_ENTRY vlc_table_b3[] =
{
	{ 0b111,       3, 60 },
	{ 0b1101,      4,  4 },
	{ 0b1100,      4,  8 },
	{ 0b1011,      4, 16 },
	{ 0b1010,      4, 32 },
	{ 0b10011,     5, 12 },
	{ 0b10010,     5, 48 },
	{ 0b10001,     5, 20 },
	{ 0b10000,     5, 40 },
	{ 0b01111,     5, 28 },
	{ 0b01110,     5, 44 },
	{ 0b01101,     5, 52 },
	{ 0b01100,     5, 56 },
	{ 0b01011,     5,  1 },
	{ 0b01010,     5, 61 },
	{ 0b01001,     5,  2 },
	{ 0b01000,     5, 62 },
	{ 0b001111,    6, 24 },
	{ 0b001110,    6, 36 },
	{ 0b001101,    6,  3 },
	{ 0b001100,    6, 63 },
	{ 0b0010111,   7,  5 },
	{ 0b0010110,   7,  9 },
	{ 0b0010101,   7, 17 },
	{ 0b0010100,   7, 33 },
	{ 0b0010011,   7,  6 },
	{ 0b0010010,   7, 10 },
	{ 0b0010001,   7, 18 },
	{ 0b0010000,   7, 34 },
	{ 0b00011111,  8,  7 },
	{ 0b00011110,  8, 11 },
	{ 0b00011101,  8, 19 },
	{ 0b00011100,  8, 35 },
	{ 0b00011011,  8, 13 },
	{ 0b00011010,  8, 49 },
	{ 0b00011001,  8, 21 },
	{ 0b00011000,  8, 41 },
	{ 0b00010111,  8, 14 },
	{ 0b00010110,  8, 50 },
	{ 0b00010101,  8, 22 },
	{ 0b00010100,  8, 42 },
	{ 0b00010011,  8, 15 },
	{ 0b00010010,  8, 51 },
	{ 0b00010001,  8, 23 },
	{ 0b00010000,  8, 43 },
	{ 0b00001111,  8, 25 },
	{ 0b00001110,  8, 37 },
	{ 0b00001101,  8, 26 },
	{ 0b00001100,  8, 38 },
	{ 0b00001011,  8, 29 },
	{ 0b00001010,  8, 45 },
	{ 0b00001001,  8, 53 },
	{ 0b00001000,  8, 57 },
	{ 0b00000111,  8, 30 },
	{ 0b00000110,  8, 46 },
	{ 0b00000101,  8, 54 },
	{ 0b00000100,  8, 58 },
	{ 0b000000111, 9, 31 },
	{ 0b000000110, 9, 47 },
	{ 0b000000101, 9, 55 },
	{ 0b000000100, 9, 59 },
	{ 0b000000011, 9, 27 },
	{ 0b000000010, 9, 39 },
	{ 0, 0, 0 },
};

const VLC_ENTRY vlc_table_b4[] =
{
	{ 0b1,           1,   0  },
	{ 0b011,         3,  -1  },
	{ 0b010,         3,   1  },
	{ 0b0011,        4,  -2  },
	{ 0b0010,        4,   2  },
	{ 0b00011,       5,  -3  },
	{ 0b00010,       5,   3  },
	{ 0b0000111,     7,  -4  },
	{ 0b0000110,     7,   4  },
	{ 0b00000111,    8,  -7  },
	{ 0b00001001,    8,  -6  },
	{ 0b00001011,    8,  -5  },
	{ 0b00001010,    8,   5  },
	{ 0b00001000,    8,   6  },
	{ 0b00000110,    8,   7  },
	{ 0b0000010011,  10, -10 },
	{ 0b0000010101,  10, -9  },
	{ 0b0000010111,  10, -8  },
	{ 0b0000010110,  10,  8  },
	{ 0b0000010100,  10,  9  },
	{ 0b0000010010,  10,  10 },
	{ 0b00000011001, 11, -16 },
	{ 0b00000011011, 11, -15 },
	{ 0b00000011101, 11, -14 },
	{ 0b00000011111, 11, -13 },
	{ 0b00000100001, 11, -12 },
	{ 0b00000100011, 11, -11 },
	{ 0b00000100010, 11,  11 },
	{ 0b00000100000, 11,  12 },
	{ 0b00000011110, 11,  13 },
	{ 0b00000011100, 11,  14 },
	{ 0b00000011010, 11,  15 },
	{ 0b00000011000, 11,  16 },
	{ 0, 0, 0 },
};

const VLC_ENTRY vlc_table_b5a[] =
{
	{ 0b00,      2, 1 },
	{ 0b01,      2, 2 },
	{ 0b100,     3, 0 },
	{ 0b101,     3, 3 },
	{ 0b110,     3, 4 },
	{ 0b1110,    4, 5 },
	{ 0b11110,   5, 6 },
	{ 0b111110,  6, 7 },
	{ 0b1111110, 7, 8 },
	{ 0, 0, 0 },
};

const VLC_ENTRY vlc_table_b5b[] =
{
	{ 0b00,       2, 0 },
	{ 0b01,       2, 1 },
	{ 0b10,       2, 2 },
	{ 0b110,      3, 3 },
	{ 0b1110,     4, 4 },
	{ 0b11110,    5, 5 },
	{ 0b111110,   6, 6 },
	{ 0b1111110,  7, 7 },
	{ 0b11111110, 8, 8 },
	{ 0, 0, 0 },
};

#define INT_SET(x, y)		(((x)*1000)+((y)+500))

const VLC_ENTRY vlc_table_dct_f[] =
{
	{ 0b10,                 2, INT_SET(0, 1) },
	{ 0b11,                 2, INT_SET(0, -1) },
	{ 0b0110,               4, INT_SET(1, 1) },
	{ 0b0111,               4, INT_SET(1, -1) },
	{ 0b01000,              5, INT_SET(0, 2) },
	{ 0b01001,              5, INT_SET(0, -2) },
	{ 0b01010,              5, INT_SET(2, 1) },
	{ 0b01011,              5, INT_SET(2, -1) },
	{ 0b001010,             6, INT_SET(0, 3) },
	{ 0b001011,             6, INT_SET(0, -3) },
	{ 0b001110,             6, INT_SET(3, 1) },
	{ 0b001111,             6, INT_SET(3, -1) },
	{ 0b001100,             6, INT_SET(4, 1) },
	{ 0b001101,             6, INT_SET(4, -1) },
	{ 0b000001,             6, INT_SET(999, 0) },	// {unsigned int(6), int(8)},
	{ 0b0001100,            7, INT_SET(1, 2) },
	{ 0b0001101,            7, INT_SET(1, -2) },
	{ 0b0001110,            7, INT_SET(5, 1) },
	{ 0b0001111,            7, INT_SET(5, -1) },
	{ 0b0001010,            7, INT_SET(6, 1) },
	{ 0b0001011,            7, INT_SET(6, -1) },
	{ 0b0001000,            7, INT_SET(7, 1) },
	{ 0b0001001,            7, INT_SET(7, -1) },
	{ 0b00001100,           8, INT_SET(0, 4) },
	{ 0b00001101,           8, INT_SET(0, -4) },
	{ 0b00001000,           8, INT_SET(2, 2) },
	{ 0b00001001,           8, INT_SET(2, -2) },
	{ 0b00001110,           8, INT_SET(8, 1) },
	{ 0b00001111,           8, INT_SET(8, -1) },
	{ 0b00001010,           8, INT_SET(9, 1) },
	{ 0b00001011,           8, INT_SET(9, -1) },
	{ 0b001001100,          9, INT_SET(0, 5) },
	{ 0b001001101,          9, INT_SET(0, -5) },
	{ 0b001000010,          9, INT_SET(0, 6) },
	{ 0b001000011,          9, INT_SET(0, -6) },
	{ 0b001001010,          9, INT_SET(1, 3) },
	{ 0b001001011,          9, INT_SET(1, -3) },
	{ 0b001001000,          9, INT_SET(3, 2) },
	{ 0b001001001,          9, INT_SET(3, -2) },
	{ 0b001001110,          9, INT_SET(10, 1) },
	{ 0b001001111,          9, INT_SET(10, -1) },
	{ 0b001000110,          9, INT_SET(11, 1) },
	{ 0b001000111,          9, INT_SET(11, -1) },
	{ 0b001000100,          9, INT_SET(12, 1) },
	{ 0b001000101,          9, INT_SET(12, -1) },
	{ 0b001000000,          9, INT_SET(13, 1) },
	{ 0b001000001,          9, INT_SET(13, -1) },
	{ 0b00000010100,       11, INT_SET(0, 7) },
	{ 0b00000010101,       11, INT_SET(0, -7) },
	{ 0b00000011000,       11, INT_SET(1, 4) },
	{ 0b00000011001,       11, INT_SET(1, -4) },
	{ 0b00000010110,       11, INT_SET(2, 3) },
	{ 0b00000010111,       11, INT_SET(2, -3) },
	{ 0b00000011110,       11, INT_SET(4, 2) },
	{ 0b00000011111,       11, INT_SET(4, -2) },
	{ 0b00000010010,       11, INT_SET(5, 2) },
	{ 0b00000010011,       11, INT_SET(5, -2) },
	{ 0b00000011100,       11, INT_SET(14, 1) },
	{ 0b00000011101,       11, INT_SET(14, -1) },
	{ 0b00000011010,       11, INT_SET(15, 1) },
	{ 0b00000011011,       11, INT_SET(15, -1) },
	{ 0b00000010000,       11, INT_SET(16, 1) },
	{ 0b00000010001,       11, INT_SET(16, -1) },
	{ 0b0000000111010,     13, INT_SET(0, 8) },
	{ 0b0000000111011,     13, INT_SET(0, -8) },
	{ 0b0000000110000,     13, INT_SET(0, 9) },
	{ 0b0000000110001,     13, INT_SET(0, -9) },
	{ 0b0000000100110,     13, INT_SET(0, 10) },
	{ 0b0000000100111,     13, INT_SET(0, -10) },
	{ 0b0000000100000,     13, INT_SET(0, 11) },
	{ 0b0000000100001,     13, INT_SET(0, -11) },
	{ 0b0000000110110,     13, INT_SET(1, 5) },
	{ 0b0000000110111,     13, INT_SET(1, -5) },
	{ 0b0000000101000,     13, INT_SET(2, 4) },
	{ 0b0000000101001,     13, INT_SET(2, -4) },
	{ 0b0000000111000,     13, INT_SET(3, 3) },
	{ 0b0000000111001,     13, INT_SET(3, -3) },
	{ 0b0000000100100,     13, INT_SET(4, 3) },
	{ 0b0000000100101,     13, INT_SET(4, -3) },
	{ 0b0000000111100,     13, INT_SET(6, 2) },
	{ 0b0000000111101,     13, INT_SET(6, -2) },
	{ 0b0000000101010,     13, INT_SET(7, 2) },
	{ 0b0000000101011,     13, INT_SET(7, -2) },
	{ 0b0000000100010,     13, INT_SET(8, 2) },
	{ 0b0000000100011,     13, INT_SET(8, -2) },
	{ 0b0000000111110,     13, INT_SET(17, 1) },
	{ 0b0000000111111,     13, INT_SET(17, -1) },
	{ 0b0000000110100,     13, INT_SET(18, 1) },
	{ 0b0000000110101,     13, INT_SET(18, -1) },
	{ 0b0000000110010,     13, INT_SET(19, 1) },
	{ 0b0000000110011,     13, INT_SET(19, -1) },
	{ 0b0000000101110,     13, INT_SET(20, 1) },
	{ 0b0000000101111,     13, INT_SET(20, -1) },
	{ 0b0000000101100,     13, INT_SET(21, 1) },
	{ 0b0000000101101,     13, INT_SET(21, -1) },
	{ 0b00000000110100,    14, INT_SET(0, 12) },
	{ 0b00000000110101,    14, INT_SET(0, -12) },
	{ 0b00000000110010,    14, INT_SET(0, 13) },
	{ 0b00000000110011,    14, INT_SET(0, -13) },
	{ 0b00000000110000,    14, INT_SET(0, 14) },
	{ 0b00000000110001,    14, INT_SET(0, -14) },
	{ 0b00000000101110,    14, INT_SET(0, 15) },
	{ 0b00000000101111,    14, INT_SET(0, -15) },
	{ 0b00000000101100,    14, INT_SET(1, 6) },
	{ 0b00000000101101,    14, INT_SET(1, -6) },
	{ 0b00000000101010,    14, INT_SET(1, 7) },
	{ 0b00000000101011,    14, INT_SET(1, -7) },
	{ 0b00000000101000,    14, INT_SET(2, 5) },
	{ 0b00000000101001,    14, INT_SET(2, -5) },
	{ 0b00000000100110,    14, INT_SET(3, 4) },
	{ 0b00000000100111,    14, INT_SET(3, -4) },
	{ 0b00000000100100,    14, INT_SET(5, 3) },
	{ 0b00000000100101,    14, INT_SET(5, -3) },
	{ 0b00000000100010,    14, INT_SET(9, 2) },
	{ 0b00000000100011,    14, INT_SET(9, -2) },
	{ 0b00000000100000,    14, INT_SET(10, 2) },
	{ 0b00000000100001,    14, INT_SET(10, -2) },
	{ 0b00000000111110,    14, INT_SET(22, 1) },
	{ 0b00000000111111,    14, INT_SET(22, -1) },
	{ 0b00000000111100,    14, INT_SET(23, 1) },
	{ 0b00000000111101,    14, INT_SET(23, -1) },
	{ 0b00000000111010,    14, INT_SET(24, 1) },
	{ 0b00000000111011,    14, INT_SET(24, -1) },
	{ 0b00000000111000,    14, INT_SET(25, 1) },
	{ 0b00000000111001,    14, INT_SET(25, -1) },
	{ 0b00000000110110,    14, INT_SET(26, 1) },
	{ 0b00000000110111,    14, INT_SET(26, -1) },
	{ 0b000000000111110,   15, INT_SET(0, 16) },
	{ 0b000000000111111,   15, INT_SET(0, -16) },
	{ 0b000000000111100,   15, INT_SET(0, 17) },
	{ 0b000000000111101,   15, INT_SET(0, -17) },
	{ 0b000000000111010,   15, INT_SET(0, 18) },
	{ 0b000000000111011,   15, INT_SET(0, -18) },
	{ 0b000000000111000,   15, INT_SET(0, 19) },
	{ 0b000000000111001,   15, INT_SET(0, -19) },
	{ 0b000000000110110,   15, INT_SET(0, 20) },
	{ 0b000000000110111,   15, INT_SET(0, -20) },
	{ 0b000000000110100,   15, INT_SET(0, 21) },
	{ 0b000000000110101,   15, INT_SET(0, -21) },
	{ 0b000000000110010,   15, INT_SET(0, 22) },
	{ 0b000000000110011,   15, INT_SET(0, -22) },
	{ 0b000000000110000,   15, INT_SET(0, 23) },
	{ 0b000000000110001,   15, INT_SET(0, -23) },
	{ 0b000000000101110,   15, INT_SET(0, 24) },
	{ 0b000000000101111,   15, INT_SET(0, -24) },
	{ 0b000000000101100,   15, INT_SET(0, 25) },
	{ 0b000000000101101,   15, INT_SET(0, -25) },
	{ 0b000000000101010,   15, INT_SET(0, 26) },
	{ 0b000000000101011,   15, INT_SET(0, -26) },
	{ 0b000000000101000,   15, INT_SET(0, 27) },
	{ 0b000000000101001,   15, INT_SET(0, -27) },
	{ 0b000000000100110,   15, INT_SET(0, 28) },
	{ 0b000000000100111,   15, INT_SET(0, -28) },
	{ 0b000000000100100,   15, INT_SET(0, 29) },
	{ 0b000000000100101,   15, INT_SET(0, -29) },
	{ 0b000000000100010,   15, INT_SET(0, 30) },
	{ 0b000000000100011,   15, INT_SET(0, -30) },
	{ 0b000000000100000,   15, INT_SET(0, 31) },
	{ 0b000000000100001,   15, INT_SET(0, -31) },
	{ 0b0000000000110000,  16, INT_SET(0, 32) },
	{ 0b0000000000110001,  16, INT_SET(0, -32) },
	{ 0b0000000000101110,  16, INT_SET(0, 33) },
	{ 0b0000000000101111,  16, INT_SET(0, -33) },
	{ 0b0000000000101100,  16, INT_SET(0, 34) },
	{ 0b0000000000101101,  16, INT_SET(0, -34) },
	{ 0b0000000000101010,  16, INT_SET(0, 35) },
	{ 0b0000000000101011,  16, INT_SET(0, -35) },
	{ 0b0000000000101000,  16, INT_SET(0, 36) },
	{ 0b0000000000101001,  16, INT_SET(0, -36) },
	{ 0b0000000000100110,  16, INT_SET(0, 37) },
	{ 0b0000000000100111,  16, INT_SET(0, -37) },
	{ 0b0000000000100100,  16, INT_SET(0, 38) },
	{ 0b0000000000100101,  16, INT_SET(0, -38) },
	{ 0b0000000000100010,  16, INT_SET(0, 39) },
	{ 0b0000000000100011,  16, INT_SET(0, -39) },
	{ 0b0000000000100000,  16, INT_SET(0, 40) },
	{ 0b0000000000100001,  16, INT_SET(0, -40) },
	{ 0b0000000000111110,  16, INT_SET(1, 8) },
	{ 0b0000000000111111,  16, INT_SET(1, -8) },
	{ 0b0000000000111100,  16, INT_SET(1, 9) },
	{ 0b0000000000111101,  16, INT_SET(1, -9) },
	{ 0b0000000000111010,  16, INT_SET(1, 10) },
	{ 0b0000000000111011,  16, INT_SET(1, -10) },
	{ 0b0000000000111000,  16, INT_SET(1, 11) },
	{ 0b0000000000111001,  16, INT_SET(1, -11) },
	{ 0b0000000000110110,  16, INT_SET(1, 12) },
	{ 0b0000000000110111,  16, INT_SET(1, -12) },
	{ 0b0000000000110100,  16, INT_SET(1, 13) },
	{ 0b0000000000110101,  16, INT_SET(1, -13) },
	{ 0b0000000000110010,  16, INT_SET(1, 14) },
	{ 0b0000000000110011,  16, INT_SET(1, -14) },
	{ 0b00000000000100110, 17, INT_SET(1, 15) },
	{ 0b00000000000100111, 17, INT_SET(1, -15) },
	{ 0b00000000000100100, 17, INT_SET(1, 16) },
	{ 0b00000000000100101, 17, INT_SET(1, -16) },
	{ 0b00000000000100010, 17, INT_SET(1, 17) },
	{ 0b00000000000100011, 17, INT_SET(1, -17) },
	{ 0b00000000000100000, 17, INT_SET(1, 18) },
	{ 0b00000000000100001, 17, INT_SET(1, -18) },
	{ 0b00000000000101000, 17, INT_SET(6, 3) },
	{ 0b00000000000101001, 17, INT_SET(6, -3) },
	{ 0b00000000000110100, 17, INT_SET(11, 2) },
	{ 0b00000000000110101, 17, INT_SET(11, -2) },
	{ 0b00000000000110010, 17, INT_SET(12, 2) },
	{ 0b00000000000110011, 17, INT_SET(12, -2) },
	{ 0b00000000000110000, 17, INT_SET(13, 2) },
	{ 0b00000000000110001, 17, INT_SET(13, -2) },
	{ 0b00000000000101110, 17, INT_SET(14, 2) },
	{ 0b00000000000101111, 17, INT_SET(14, -2) },
	{ 0b00000000000101100, 17, INT_SET(15, 2) },
	{ 0b00000000000101101, 17, INT_SET(15, -2) },
	{ 0b00000000000101010, 17, INT_SET(16, 2) },
	{ 0b00000000000101011, 17, INT_SET(16, -2) },
	{ 0b00000000000111110, 17, INT_SET(27, 1) },
	{ 0b00000000000111111, 17, INT_SET(27, -1) },
	{ 0b00000000000111100, 17, INT_SET(28, 1) },
	{ 0b00000000000111101, 17, INT_SET(28, -1) },
	{ 0b00000000000111010, 17, INT_SET(29, 1) },
	{ 0b00000000000111011, 17, INT_SET(29, -1) },
	{ 0b00000000000111000, 17, INT_SET(30, 1) },
	{ 0b00000000000111001, 17, INT_SET(30, -1) },
	{ 0b00000000000110110, 17, INT_SET(31, 1) },
	{ 0b00000000000110111, 17, INT_SET(31, -1) },
	{ 0, 0, 0 },
};

const VLC_ENTRY vlc_table_dct_n[] =
{
	{ 0b110,                3, INT_SET(0, 1) },
	{ 0b111,                3, INT_SET(0, -1) },
	{ 0b0110,               4, INT_SET(1, 1) },
	{ 0b0111,               4, INT_SET(1, -1) },
	{ 0b01000,              5, INT_SET(0, 2) },
	{ 0b01001,              5, INT_SET(0, -2) },
	{ 0b01010,              5, INT_SET(2, 1) },
	{ 0b01011,              5, INT_SET(2, -1) },
	{ 0b001010,             6, INT_SET(0, 3) },
	{ 0b001011,             6, INT_SET(0, -3) },
	{ 0b001110,             6, INT_SET(3, 1) },
	{ 0b001111,             6, INT_SET(3, -1) },
	{ 0b001100,             6, INT_SET(4, 1) },
	{ 0b001101,             6, INT_SET(4, -1) },
	{ 0b000001,             6, INT_SET(999, 0) },	// {unsigned int(6), int(8)}
	{ 0b0001100,            7, INT_SET(1, 2) },
	{ 0b0001101,            7, INT_SET(1, -2) },
	{ 0b0001110,            7, INT_SET(5, 1) },
	{ 0b0001111,            7, INT_SET(5, -1) },
	{ 0b0001010,            7, INT_SET(6, 1) },
	{ 0b0001011,            7, INT_SET(6, -1) },
	{ 0b0001000,            7, INT_SET(7, 1) },
	{ 0b0001001,            7, INT_SET(7, -1) },
	{ 0b00001100,           8, INT_SET(0, 4) },
	{ 0b00001101,           8, INT_SET(0, -4) },
	{ 0b00001000,           8, INT_SET(2, 2) },
	{ 0b00001001,           8, INT_SET(2, -2) },
	{ 0b00001110,           8, INT_SET(8, 1) },
	{ 0b00001111,           8, INT_SET(8, -1) },
	{ 0b00001010,           8, INT_SET(9, 1) },
	{ 0b00001011,           8, INT_SET(9, -1) },
	{ 0b001001100,          9, INT_SET(0, 5) },
	{ 0b001001101,          9, INT_SET(0, -5) },
	{ 0b001000010,          9, INT_SET(0, 6) },
	{ 0b001000011,          9, INT_SET(0, -6) },
	{ 0b001001010,          9, INT_SET(1, 3) },
	{ 0b001001011,          9, INT_SET(1, -3) },
	{ 0b001001000,          9, INT_SET(3, 2) },
	{ 0b001001001,          9, INT_SET(3, -2) },
	{ 0b001001110,          9, INT_SET(10, 1) },
	{ 0b001001111,          9, INT_SET(10, -1) },
	{ 0b001000110,          9, INT_SET(11, 1) },
	{ 0b001000111,          9, INT_SET(11, -1) },
	{ 0b001000100,          9, INT_SET(12, 1) },
	{ 0b001000101,          9, INT_SET(12, -1) },
	{ 0b001000000,          9, INT_SET(13, 1) },
	{ 0b001000001,          9, INT_SET(13, -1) },
	{ 0b00000010100,       11, INT_SET(0, 7) },
	{ 0b00000010101,       11, INT_SET(0, -7) },
	{ 0b00000011000,       11, INT_SET(1, 4) },
	{ 0b00000011001,       11, INT_SET(1, -4) },
	{ 0b00000010110,       11, INT_SET(2, 3) },
	{ 0b00000010111,       11, INT_SET(2, -3) },
	{ 0b00000011110,       11, INT_SET(4, 2) },
	{ 0b00000011111,       11, INT_SET(4, -2) },
	{ 0b00000010010,       11, INT_SET(5, 2) },
	{ 0b00000010011,       11, INT_SET(5, -2) },
	{ 0b00000011100,       11, INT_SET(14, 1) },
	{ 0b00000011101,       11, INT_SET(14, -1) },
	{ 0b00000011010,       11, INT_SET(15, 1) },
	{ 0b00000011011,       11, INT_SET(15, -1) },
	{ 0b00000010000,       11, INT_SET(16, 1) },
	{ 0b00000010001,       11, INT_SET(16, -1) },
	{ 0b0000000111010,     13, INT_SET(0, 8) },
	{ 0b0000000111011,     13, INT_SET(0, -8) },
	{ 0b0000000110000,     13, INT_SET(0, 9) },
	{ 0b0000000110001,     13, INT_SET(0, -9) },
	{ 0b0000000100110,     13, INT_SET(0, 10) },
	{ 0b0000000100111,     13, INT_SET(0, -10) },
	{ 0b0000000100000,     13, INT_SET(0, 11) },
	{ 0b0000000100001,     13, INT_SET(0, -11) },
	{ 0b0000000110110,     13, INT_SET(1, 5) },
	{ 0b0000000110111,     13, INT_SET(1, -5) },
	{ 0b0000000101000,     13, INT_SET(2, 4) },
	{ 0b0000000101001,     13, INT_SET(2, -4) },
	{ 0b0000000111000,     13, INT_SET(3, 3) },
	{ 0b0000000111001,     13, INT_SET(3, -3) },
	{ 0b0000000100100,     13, INT_SET(4, 3) },
	{ 0b0000000100101,     13, INT_SET(4, -3) },
	{ 0b0000000111100,     13, INT_SET(6, 2) },
	{ 0b0000000111101,     13, INT_SET(6, -2) },
	{ 0b0000000101010,     13, INT_SET(7, 2) },
	{ 0b0000000101011,     13, INT_SET(7, -2) },
	{ 0b0000000100010,     13, INT_SET(8, 2) },
	{ 0b0000000100011,     13, INT_SET(8, -2) },
	{ 0b0000000111110,     13, INT_SET(17, 1) },
	{ 0b0000000111111,     13, INT_SET(17, -1) },
	{ 0b0000000110100,     13, INT_SET(18, 1) },
	{ 0b0000000110101,     13, INT_SET(18, -1) },
	{ 0b0000000110010,     13, INT_SET(19, 1) },
	{ 0b0000000110011,     13, INT_SET(19, -1) },
	{ 0b0000000101110,     13, INT_SET(20, 1) },
	{ 0b0000000101111,     13, INT_SET(20, -1) },
	{ 0b0000000101100,     13, INT_SET(21, 1) },
	{ 0b0000000101101,     13, INT_SET(21, -1) },
	{ 0b00000000110100,    14, INT_SET(0, 12) },
	{ 0b00000000110101,    14, INT_SET(0, -12) },
	{ 0b00000000110010,    14, INT_SET(0, 13) },
	{ 0b00000000110011,    14, INT_SET(0, -13) },
	{ 0b00000000110000,    14, INT_SET(0, 14) },
	{ 0b00000000110001,    14, INT_SET(0, -14) },
	{ 0b00000000101110,    14, INT_SET(0, 15) },
	{ 0b00000000101111,    14, INT_SET(0, -15) },
	{ 0b00000000101100,    14, INT_SET(1, 6) },
	{ 0b00000000101101,    14, INT_SET(1, -6) },
	{ 0b00000000101010,    14, INT_SET(1, 7) },
	{ 0b00000000101011,    14, INT_SET(1, -7) },
	{ 0b00000000101000,    14, INT_SET(2, 5) },
	{ 0b00000000101001,    14, INT_SET(2, -5) },
	{ 0b00000000100110,    14, INT_SET(3, 4) },
	{ 0b00000000100111,    14, INT_SET(3, -4) },
	{ 0b00000000100100,    14, INT_SET(5, 3) },
	{ 0b00000000100101,    14, INT_SET(5, -3) },
	{ 0b00000000100010,    14, INT_SET(9, 2) },
	{ 0b00000000100011,    14, INT_SET(9, -2) },
	{ 0b00000000100000,    14, INT_SET(10, 2) },
	{ 0b00000000100001,    14, INT_SET(10, -2) },
	{ 0b00000000111110,    14, INT_SET(22, 1) },
	{ 0b00000000111111,    14, INT_SET(22, -1) },
	{ 0b00000000111100,    14, INT_SET(23, 1) },
	{ 0b00000000111101,    14, INT_SET(23, -1) },
	{ 0b00000000111010,    14, INT_SET(24, 1) },
	{ 0b00000000111011,    14, INT_SET(24, -1) },
	{ 0b00000000111000,    14, INT_SET(25, 1) },
	{ 0b00000000111001,    14, INT_SET(25, -1) },
	{ 0b00000000110110,    14, INT_SET(26, 1) },
	{ 0b00000000110111,    14, INT_SET(26, -1) },
	{ 0b000000000111110,   15, INT_SET(0, 16) },
	{ 0b000000000111111,   15, INT_SET(0, -16) },
	{ 0b000000000111100,   15, INT_SET(0, 17) },
	{ 0b000000000111101,   15, INT_SET(0, -17) },
	{ 0b000000000111010,   15, INT_SET(0, 18) },
	{ 0b000000000111011,   15, INT_SET(0, -18) },
	{ 0b000000000111000,   15, INT_SET(0, 19) },
	{ 0b000000000111001,   15, INT_SET(0, -19) },
	{ 0b000000000110110,   15, INT_SET(0, 20) },
	{ 0b000000000110111,   15, INT_SET(0, -20) },
	{ 0b000000000110100,   15, INT_SET(0, 21) },
	{ 0b000000000110101,   15, INT_SET(0, -21) },
	{ 0b000000000110010,   15, INT_SET(0, 22) },
	{ 0b000000000110011,   15, INT_SET(0, -22) },
	{ 0b000000000110000,   15, INT_SET(0, 23) },
	{ 0b000000000110001,   15, INT_SET(0, -23) },
	{ 0b000000000101110,   15, INT_SET(0, 24) },
	{ 0b000000000101111,   15, INT_SET(0, -24) },
	{ 0b000000000101100,   15, INT_SET(0, 25) },
	{ 0b000000000101101,   15, INT_SET(0, -25) },
	{ 0b000000000101010,   15, INT_SET(0, 26) },
	{ 0b000000000101011,   15, INT_SET(0, -26) },
	{ 0b000000000101000,   15, INT_SET(0, 27) },
	{ 0b000000000101001,   15, INT_SET(0, -27) },
	{ 0b000000000100110,   15, INT_SET(0, 28) },
	{ 0b000000000100111,   15, INT_SET(0, -28) },
	{ 0b000000000100100,   15, INT_SET(0, 29) },
	{ 0b000000000100101,   15, INT_SET(0, -29) },
	{ 0b000000000100010,   15, INT_SET(0, 30) },
	{ 0b000000000100011,   15, INT_SET(0, -30) },
	{ 0b000000000100000,   15, INT_SET(0, 31) },
	{ 0b000000000100001,   15, INT_SET(0, -31) },
	{ 0b0000000000110000,  16, INT_SET(0, 32) },
	{ 0b0000000000110001,  16, INT_SET(0, -32) },
	{ 0b0000000000101110,  16, INT_SET(0, 33) },
	{ 0b0000000000101111,  16, INT_SET(0, -33) },
	{ 0b0000000000101100,  16, INT_SET(0, 34) },
	{ 0b0000000000101101,  16, INT_SET(0, -34) },
	{ 0b0000000000101010,  16, INT_SET(0, 35) },
	{ 0b0000000000101011,  16, INT_SET(0, -35) },
	{ 0b0000000000101000,  16, INT_SET(0, 36) },
	{ 0b0000000000101001,  16, INT_SET(0, -36) },
	{ 0b0000000000100110,  16, INT_SET(0, 37) },
	{ 0b0000000000100111,  16, INT_SET(0, -37) },
	{ 0b0000000000100100,  16, INT_SET(0, 38) },
	{ 0b0000000000100101,  16, INT_SET(0, -38) },
	{ 0b0000000000100010,  16, INT_SET(0, 39) },
	{ 0b0000000000100011,  16, INT_SET(0, -39) },
	{ 0b0000000000100000,  16, INT_SET(0, 40) },
	{ 0b0000000000100001,  16, INT_SET(0, -40) },
	{ 0b0000000000111110,  16, INT_SET(1, 8) },
	{ 0b0000000000111111,  16, INT_SET(1, -8) },
	{ 0b0000000000111100,  16, INT_SET(1, 9) },
	{ 0b0000000000111101,  16, INT_SET(1, -9) },
	{ 0b0000000000111010,  16, INT_SET(1, 10) },
	{ 0b0000000000111011,  16, INT_SET(1, -10) },
	{ 0b0000000000111000,  16, INT_SET(1, 11) },
	{ 0b0000000000111001,  16, INT_SET(1, -11) },
	{ 0b0000000000110110,  16, INT_SET(1, 12) },
	{ 0b0000000000110111,  16, INT_SET(1, -12) },
	{ 0b0000000000110100,  16, INT_SET(1, 13) },
	{ 0b0000000000110101,  16, INT_SET(1, -13) },
	{ 0b0000000000110010,  16, INT_SET(1, 14) },
	{ 0b0000000000110011,  16, INT_SET(1, -14) },
	{ 0b00000000000100110, 17, INT_SET(1, 15) },
	{ 0b00000000000100111, 17, INT_SET(1, -15) },
	{ 0b00000000000100100, 17, INT_SET(1, 16) },
	{ 0b00000000000100101, 17, INT_SET(1, -16) },
	{ 0b00000000000100010, 17, INT_SET(1, 17) },
	{ 0b00000000000100011, 17, INT_SET(1, -17) },
	{ 0b00000000000100000, 17, INT_SET(1, 18) },
	{ 0b00000000000100001, 17, INT_SET(1, -18) },
	{ 0b00000000000101000, 17, INT_SET(6, 3) },
	{ 0b00000000000101001, 17, INT_SET(6, -3) },
	{ 0b00000000000110100, 17, INT_SET(11, 2) },
	{ 0b00000000000110101, 17, INT_SET(11, -2) },
	{ 0b00000000000110010, 17, INT_SET(12, 2) },
	{ 0b00000000000110011, 17, INT_SET(12, -2) },
	{ 0b00000000000110000, 17, INT_SET(13, 2) },
	{ 0b00000000000110001, 17, INT_SET(13, -2) },
	{ 0b00000000000101110, 17, INT_SET(14, 2) },
	{ 0b00000000000101111, 17, INT_SET(14, -2) },
	{ 0b00000000000101100, 17, INT_SET(15, 2) },
	{ 0b00000000000101101, 17, INT_SET(15, -2) },
	{ 0b00000000000101010, 17, INT_SET(16, 2) },
	{ 0b00000000000101011, 17, INT_SET(16, -2) },
	{ 0b00000000000111110, 17, INT_SET(27, 1) },
	{ 0b00000000000111111, 17, INT_SET(27, -1) },
	{ 0b00000000000111100, 17, INT_SET(28, 1) },
	{ 0b00000000000111101, 17, INT_SET(28, -1) },
	{ 0b00000000000111010, 17, INT_SET(29, 1) },
	{ 0b00000000000111011, 17, INT_SET(29, -1) },
	{ 0b00000000000111000, 17, INT_SET(30, 1) },
	{ 0b00000000000111001, 17, INT_SET(30, -1) },
	{ 0b00000000000110110, 17, INT_SET(31, 1) },
	{ 0b00000000000110111, 17, INT_SET(31, -1) },
	{ 0, 0, 0 },
};
