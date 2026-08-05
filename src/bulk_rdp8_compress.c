/**
 * RDP8 bulk compressor
 *
 * Copyright 2015-2026 Jay Sorg <jay.sorg@gmail.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bulk_rdp8_compress.h>

#include "bulk_common_private.h"

#define HASH_TABLE_WIDTH        65536
#define HIST_BUF_LEN            2500000
#define BUCKET_DEPTH            4
#define MAX_UNENCODED_LITERALS  (1024 * 30)

struct token
{
    unsigned int code;      /* Huffman code */
    int code_bits;          /* numbesr of bits in Huffman code */
    int value_bits;         /* number of bits in value */
    int value_base;         /* value to add (decoder) or subtract (encoder) */
};

/* Table from MS-RDPEGFX 3.1.9.1.2.4 Compressed Segment Trailer */
static struct token g_dist_tokens[] =
{
    {17,  5, 5, 0},             //    1_0001    Match distance 1...31
    {18,  5, 7, 32},            //    1_0010    Match distance 32...159
    {19,  5, 9, 160},           //    1_0011    Match distance 160...671
    {20,  5, 10, 672},          //    1_0100    Match distance 672...1695
    {21,  5, 12, 1696},         //    1_0101    Match distance 1696...5791
    {44,  6, 14, 5792},         //   10_1100    Match distance 5792...22175
    {45,  6, 15, 22176},        //   10_1101    Match distance 22176...54943
    {92,  7, 18, 54944},        //  101_1100    Match distance 54944...317087
    {93,  7, 20, 317088},       //  101_1101    Match distance 317088...1365663
    {188, 8, 20, 1365664},      // 1011_1100    Match distance 1365664...2414239
    {189, 8, 21, 2414240},      // 1011_1101    Match distance 2414240...2500000
    {0, 0, 0, 0}
};

/* Table from MS-RDPEGFX 3.1.9.1.2.4 Compressed Segment Trailer */
static struct token g_lom_tokens[] =
{
    {0x00, 1, 0, 0},            //                  0   Length 3
    {0x02, 2, 2, 4},            //                 10   Length 4...7
    {0x06, 3, 3, 8},            //                110   Length 8...15
    {0x0E, 4, 4, 16},           //               1110   Length 16...31
    {0x1E, 5, 5, 32},           //             1_1110   Length 32...63
    {0x3E, 6, 6, 64},           //            11_1110   Length 64...127
    {0x7E, 7, 7, 128},          //           111_1110   Length 128...255
    {0xFE, 8, 8, 256},          //          1111_1110   Length 256...511
    {0x1FE, 9, 9, 512},         //        1_1111_1110   Length 512...1023
    {0x3FE, 10, 10, 1024},      //       11_1111_1110   Length 1024...2047
    {0x7FE, 11, 11, 2048},      //      111_1111_1110   Length 2048...4095
    {0xFFE, 12, 12, 4096},      //     1111_1111_1110   Length 4096...8191
    {0x1FFE, 13, 13, 8192},     //   1_1111_1111_1110   Length 8192...16383
    {0x3FFE, 14, 14, 16384},    //  11_1111_1111_1110   Length 16384...32767
    {0x7FFE, 15, 15, 32768},    // 111_1111_1111_1110   Length 32768...65535
    {0, 0, 0, 0}
};

/* Table from MS-RDPEGFX 3.1.9.1.2.4 Compressed Segment Trailer */
static struct token g_literals[256] =
{
    {0x18, 5, 0, 0},  /* 0x00    1_1000 */  {0x19, 5, 0, 0},  /* 0x01    1_1001 */
    {0x34, 6, 0, 0},  /* 0x02   11_0100 */  {0x35, 6, 0, 0},  /* 0x03   11_0101 */
    {0x6e, 7, 0, 0},  /* 0x04  110_1110 */  {0x6f, 7, 0, 0},  /* 0x05  110_1111 */
    {0x70, 7, 0, 0},  /* 0x06  111_0000 */  {0x71, 7, 0, 0},  /* 0x07  111_0001 */
    {0x72, 7, 0, 0},  /* 0x08  111_0010 */  {0x73, 7, 0, 0},  /* 0x09  111_0011 */
    {0x74, 7, 0, 0},  /* 0x0A  111_0100 */  {0x75, 7, 0, 0},  /* 0x0B  111_0101 */
    {0xFC, 8, 0, 0},  /* 0x0C 1111_1100 */  {0x0D, 9, 0, 0},  /* 0x0D           */
    {0x0E, 9, 0, 0},  /* 0x0E           */  {0x0F, 9, 0, 0},  /* 0x0F           */
    {0x10, 9, 0, 0},  /* 0x10           */  {0x11, 9, 0, 0},  /* 0x11           */
    {0x12, 9, 0, 0},  /* 0x12           */  {0x13, 9, 0, 0},  /* 0x13           */
    {0x14, 9, 0, 0},  /* 0x14           */  {0x15, 9, 0, 0},  /* 0x15           */
    {0x16, 9, 0, 0},  /* 0x16           */  {0x17, 9, 0, 0},  /* 0x17           */
    {0x18, 9, 0, 0},  /* 0x18           */  {0x19, 9, 0, 0},  /* 0x19           */
    {0x1A, 9, 0, 0},  /* 0x1A           */  {0x1B, 9, 0, 0},  /* 0x1B           */
    {0x1C, 9, 0, 0},  /* 0x1C           */  {0x1D, 9, 0, 0},  /* 0x1D           */
    {0x1E, 9, 0, 0},  /* 0x1E           */  {0x1F, 9, 0, 0},  /* 0x1F           */
    {0x20, 9, 0, 0},  /* 0x20           */  {0x21, 9, 0, 0},  /* 0x21           */
    {0x22, 9, 0, 0},  /* 0x22           */  {0x23, 9, 0, 0},  /* 0x23           */
    {0x24, 9, 0, 0},  /* 0x24           */  {0x25, 9, 0, 0},  /* 0x25           */
    {0x26, 9, 0, 0},  /* 0x26           */  {0x27, 9, 0, 0},  /* 0x27           */
    {0x28, 9, 0, 0},  /* 0x28           */  {0x29, 9, 0, 0},  /* 0x29           */
    {0x2A, 9, 0, 0},  /* 0x2A           */  {0x2B, 9, 0, 0},  /* 0x2B           */
    {0x2C, 9, 0, 0},  /* 0x2C           */  {0x2D, 9, 0, 0},  /* 0x2D           */
    {0x2E, 9, 0, 0},  /* 0x2E           */  {0x2F, 9, 0, 0},  /* 0x2F           */
    {0x30, 9, 0, 0},  /* 0x30           */  {0x31, 9, 0, 0},  /* 0x31           */
    {0x32, 9, 0, 0},  /* 0x32           */  {0x33, 9, 0, 0},  /* 0x33           */
    {0x34, 9, 0, 0},  /* 0x34           */  {0x35, 9, 0, 0},  /* 0x35           */
    {0x36, 9, 0, 0},  /* 0x36           */  {0x37, 9, 0, 0},  /* 0x37           */
    {0xFD, 8, 0, 0},  /* 0x38 1111_1101 */  {0xFE, 8, 0, 0},  /* 0x39 1111_1110 */
    {0x76, 7, 0, 0},  /* 0x3A  111_0110 */  {0x77, 7, 0, 0},  /* 0x3B  111_0111 */
    {0x78, 7, 0, 0},  /* 0x3C  111_1000 */  {0x79, 7, 0, 0},  /* 0x3D  111_1001 */
    {0x7A, 7, 0, 0},  /* 0x3E  111_1010 */  {0x7B, 7, 0, 0},  /* 0x3F  111_1011 */
    {0x7C, 7, 0, 0},  /* 0x40  111_1100 */  {0x41, 9, 0, 0},  /* 0x41           */
    {0x42, 9, 0, 0},  /* 0x42           */  {0x43, 9, 0, 0},  /* 0x43           */
    {0x44, 9, 0, 0},  /* 0x44           */  {0x45, 9, 0, 0},  /* 0x45           */
    {0x46, 9, 0, 0},  /* 0x46           */  {0x47, 9, 0, 0},  /* 0x47           */
    {0x48, 9, 0, 0},  /* 0x48           */  {0x49, 9, 0, 0},  /* 0x49           */
    {0x4A, 9, 0, 0},  /* 0x4A           */  {0x4B, 9, 0, 0},  /* 0x4B           */
    {0x4C, 9, 0, 0},  /* 0x4C           */  {0x4D, 9, 0, 0},  /* 0x4D           */
    {0x4E, 9, 0, 0},  /* 0x4E           */  {0x4F, 9, 0, 0},  /* 0x4F           */
    {0x50, 9, 0, 0},  /* 0x50           */  {0x51, 9, 0, 0},  /* 0x51           */
    {0x52, 9, 0, 0},  /* 0x52           */  {0x53, 9, 0, 0},  /* 0x53           */
    {0x54, 9, 0, 0},  /* 0x54           */  {0x55, 9, 0, 0},  /* 0x55           */
    {0x56, 9, 0, 0},  /* 0x56           */  {0x57, 9, 0, 0},  /* 0x57           */
    {0x58, 9, 0, 0},  /* 0x58           */  {0x59, 9, 0, 0},  /* 0x59           */
    {0x5A, 9, 0, 0},  /* 0x5A           */  {0x5B, 9, 0, 0},  /* 0x5B           */
    {0x5C, 9, 0, 0},  /* 0x5C           */  {0x5D, 9, 0, 0},  /* 0x5D           */
    {0x5E, 9, 0, 0},  /* 0x5E           */  {0x5F, 9, 0, 0},  /* 0x5F           */
    {0x60, 9, 0, 0},  /* 0x60           */  {0x61, 9, 0, 0},  /* 0x61           */
    {0x62, 9, 0, 0},  /* 0x62           */  {0x63, 9, 0, 0},  /* 0x63           */
    {0x64, 9, 0, 0},  /* 0x64           */  {0x65, 9, 0, 0},  /* 0x65           */
    {0xFF, 8, 0, 0},  /* 0x66 1111_1111 */  {0x67, 9, 0, 0},  /* 0x67           */
    {0x68, 9, 0, 0},  /* 0x68           */  {0x69, 9, 0, 0},  /* 0x69           */
    {0x6A, 9, 0, 0},  /* 0x6A           */  {0x6B, 9, 0, 0},  /* 0x6B           */
    {0x6C, 9, 0, 0},  /* 0x6C           */  {0x6D, 9, 0, 0},  /* 0x6D           */
    {0x6E, 9, 0, 0},  /* 0x6E           */  {0x6F, 9, 0, 0},  /* 0x6F           */
    {0x70, 9, 0, 0},  /* 0x70           */  {0x71, 9, 0, 0},  /* 0x71           */
    {0x72, 9, 0, 0},  /* 0x72           */  {0x73, 9, 0, 0},  /* 0x73           */
    {0x74, 9, 0, 0},  /* 0x74           */  {0x75, 9, 0, 0},  /* 0x75           */
    {0x76, 9, 0, 0},  /* 0x76           */  {0x77, 9, 0, 0},  /* 0x77           */
    {0x78, 9, 0, 0},  /* 0x78           */  {0x79, 9, 0, 0},  /* 0x79           */
    {0x7A, 9, 0, 0},  /* 0x7A           */  {0x7B, 9, 0, 0},  /* 0x7B           */
    {0x7C, 9, 0, 0},  /* 0x7C           */  {0x7D, 9, 0, 0},  /* 0x7D           */
    {0x7E, 9, 0, 0},  /* 0x7E           */  {0x7F, 9, 0, 0},  /* 0x7F           */
    {0x7D, 7, 0, 0},  /* 0x80 111_1101  */  {0x81, 9, 0, 0},  /* 0x81           */
    {0x82, 9, 0, 0},  /* 0x82           */  {0x83, 9, 0, 0},  /* 0x83           */
    {0x84, 9, 0, 0},  /* 0x84           */  {0x85, 9, 0, 0},  /* 0x85           */
    {0x86, 9, 0, 0},  /* 0x86           */  {0x87, 9, 0, 0},  /* 0x87           */
    {0x88, 9, 0, 0},  /* 0x88           */  {0x89, 9, 0, 0},  /* 0x89           */
    {0x8A, 9, 0, 0},  /* 0x8A           */  {0x8B, 9, 0, 0},  /* 0x8B           */
    {0x8C, 9, 0, 0},  /* 0x8C           */  {0x8D, 9, 0, 0},  /* 0x8D           */
    {0x8E, 9, 0, 0},  /* 0x8E           */  {0x8F, 9, 0, 0},  /* 0x8F           */
    {0x90, 9, 0, 0},  /* 0x90           */  {0x91, 9, 0, 0},  /* 0x91           */
    {0x92, 9, 0, 0},  /* 0x92           */  {0x93, 9, 0, 0},  /* 0x93           */
    {0x94, 9, 0, 0},  /* 0x94           */  {0x95, 9, 0, 0},  /* 0x95           */
    {0x96, 9, 0, 0},  /* 0x96           */  {0x97, 9, 0, 0},  /* 0x97           */
    {0x98, 9, 0, 0},  /* 0x98           */  {0x99, 9, 0, 0},  /* 0x99           */
    {0x9A, 9, 0, 0},  /* 0x9A           */  {0x9B, 9, 0, 0},  /* 0x9B           */
    {0x9C, 9, 0, 0},  /* 0x9C           */  {0x9D, 9, 0, 0},  /* 0x9D           */
    {0x9E, 9, 0, 0},  /* 0x9E           */  {0x9F, 9, 0, 0},  /* 0x9F           */
    {0xA0, 9, 0, 0},  /* 0xA0           */  {0xA1, 9, 0, 0},  /* 0xA1           */
    {0xA2, 9, 0, 0},  /* 0xA2           */  {0xA3, 9, 0, 0},  /* 0xA3           */
    {0xA4, 9, 0, 0},  /* 0xA4           */  {0xA5, 9, 0, 0},  /* 0xA5           */
    {0xA6, 9, 0, 0},  /* 0xA6           */  {0xA7, 9, 0, 0},  /* 0xA7           */
    {0xA8, 9, 0, 0},  /* 0xA8           */  {0xA9, 9, 0, 0},  /* 0xA9           */
    {0xAA, 9, 0, 0},  /* 0xAA           */  {0xAB, 9, 0, 0},  /* 0xAB           */
    {0xAC, 9, 0, 0},  /* 0xAC           */  {0xAD, 9, 0, 0},  /* 0xAD           */
    {0xAE, 9, 0, 0},  /* 0xAE           */  {0xAF, 9, 0, 0},  /* 0xAF           */
    {0xB0, 9, 0, 0},  /* 0xB0           */  {0xB1, 9, 0, 0},  /* 0xB1           */
    {0xB2, 9, 0, 0},  /* 0xB2           */  {0xB3, 9, 0, 0},  /* 0xB3           */
    {0xB4, 9, 0, 0},  /* 0xB4           */  {0xB5, 9, 0, 0},  /* 0xB5           */
    {0xB6, 9, 0, 0},  /* 0xB6           */  {0xB7, 9, 0, 0},  /* 0xB7           */
    {0xB8, 9, 0, 0},  /* 0xB8           */  {0xB9, 9, 0, 0},  /* 0xB9           */
    {0xBA, 9, 0, 0},  /* 0xBA           */  {0xBB, 9, 0, 0},  /* 0xBB           */
    {0xBC, 9, 0, 0},  /* 0xBC           */  {0xBD, 9, 0, 0},  /* 0xBD           */
    {0xBE, 9, 0, 0},  /* 0xBE           */  {0xBF, 9, 0, 0},  /* 0xBF           */
    {0xC0, 9, 0, 0},  /* 0xC0           */  {0xC1, 9, 0, 0},  /* 0xC1           */
    {0xC2, 9, 0, 0},  /* 0xC2           */  {0xC3, 9, 0, 0},  /* 0xC3           */
    {0xC4, 9, 0, 0},  /* 0xC4           */  {0xC5, 9, 0, 0},  /* 0xC5           */
    {0xC6, 9, 0, 0},  /* 0xC6           */  {0xC7, 9, 0, 0},  /* 0xC7           */
    {0xC8, 9, 0, 0},  /* 0xC8           */  {0xC9, 9, 0, 0},  /* 0xC9           */
    {0xCA, 9, 0, 0},  /* 0xCA           */  {0xCB, 9, 0, 0},  /* 0xCB           */
    {0xCC, 9, 0, 0},  /* 0xCC           */  {0xCD, 9, 0, 0},  /* 0xCD           */
    {0xCE, 9, 0, 0},  /* 0xCE           */  {0xCF, 9, 0, 0},  /* 0xCF           */
    {0xD0, 9, 0, 0},  /* 0xD0           */  {0xD1, 9, 0, 0},  /* 0xD1           */
    {0xD2, 9, 0, 0},  /* 0xD2           */  {0xD3, 9, 0, 0},  /* 0xD3           */
    {0xD4, 9, 0, 0},  /* 0xD4           */  {0xD5, 9, 0, 0},  /* 0xD5           */
    {0xD6, 9, 0, 0},  /* 0xD6           */  {0xD7, 9, 0, 0},  /* 0xD7           */
    {0xD8, 9, 0, 0},  /* 0xD8           */  {0xD9, 9, 0, 0},  /* 0xD9           */
    {0xDA, 9, 0, 0},  /* 0xDA           */  {0xDB, 9, 0, 0},  /* 0xDB           */
    {0xDC, 9, 0, 0},  /* 0xDC           */  {0xDD, 9, 0, 0},  /* 0xDD           */
    {0xDE, 9, 0, 0},  /* 0xDE           */  {0xDF, 9, 0, 0},  /* 0xDF           */
    {0xE0, 9, 0, 0},  /* 0xE0           */  {0xE1, 9, 0, 0},  /* 0xE1           */
    {0xE2, 9, 0, 0},  /* 0xE2           */  {0xE3, 9, 0, 0},  /* 0xE3           */
    {0xE4, 9, 0, 0},  /* 0xE4           */  {0xE5, 9, 0, 0},  /* 0xE5           */
    {0xE6, 9, 0, 0},  /* 0xE6           */  {0xE7, 9, 0, 0},  /* 0xE7           */
    {0xE8, 9, 0, 0},  /* 0xE8           */  {0xE9, 9, 0, 0},  /* 0xE9           */
    {0xEA, 9, 0, 0},  /* 0xEA           */  {0xEB, 9, 0, 0},  /* 0xEB           */
    {0xEC, 9, 0, 0},  /* 0xEC           */  {0xED, 9, 0, 0},  /* 0xED           */
    {0xEE, 9, 0, 0},  /* 0xEE           */  {0xEF, 9, 0, 0},  /* 0xEF           */
    {0xF0, 9, 0, 0},  /* 0xF0           */  {0xF1, 9, 0, 0},  /* 0xF1           */
    {0xF2, 9, 0, 0},  /* 0xF2           */  {0xF3, 9, 0, 0},  /* 0xF3           */
    {0xF4, 9, 0, 0},  /* 0xF4           */  {0xF5, 9, 0, 0},  /* 0xF5           */
    {0xF6, 9, 0, 0},  /* 0xF6           */  {0xF7, 9, 0, 0},  /* 0xF7           */
    {0xF8, 9, 0, 0},  /* 0xF8           */  {0xF9, 9, 0, 0},  /* 0xF9           */
    {0xFA, 9, 0, 0},  /* 0xFA           */  {0xFB, 9, 0, 0},  /* 0xFB           */
    {0xFC, 9, 0, 0},  /* 0xFC           */  {0xFD, 9, 0, 0},  /* 0xFD           */
    {0xFE, 9, 0, 0},  /* 0xFE           */  {0x36, 6, 0, 0},  /* 0xFF 11_0110   */
};

struct bit_writer
{
    unsigned char *buf; /* output byte buffer */
    int  index;         /* next byte to write */
    unsigned int data;  /* 32-bit bit cache */
    int bits_left;      /* free bits remaining in 'data' (MSB-first) */
};

/*****************************************************************************/
static void
bw_init(struct bit_writer *bw, unsigned char *buf)
{
    if ((bw == NULL) || (buf == NULL))
    {
        return;
    }
    bw->buf = buf;
    bw->index = 0;
    bw->data = 0;
    bw->bits_left = 32;
}

/*****************************************************************************/
static void
bw_update_data(struct bit_writer *bw)
{
    /* write data to output buf */
    bw->buf[bw->index++] = bw->data >> 24;
    bw->buf[bw->index++] = bw->data >> 16;
    bw->buf[bw->index++] = bw->data >> 8;
    bw->buf[bw->index++] = bw->data;
    bw->data = 0;
}

/*****************************************************************************/
static void
bw_put_bits(struct bit_writer *bw, unsigned int value, int nbits)
{
    int first;
    int second;

    if (nbits <= 0)
    {
        return;
    }
    /* Mask value so only the lowest nbits remain */
    if (nbits < 32)
    {
        value &= ((1u << nbits) - 1u);
    }
    /* Not enough room in current 32-bit word: split */
    if (bw->bits_left < nbits)
    {
        first = bw->bits_left;          /* bits we can write now */
        second = nbits - first;         /* bits remaining */
        /* Write high part into current data */
        bw->data |= value >> second;
        bw_update_data(bw);             /* flush 32-bit word */
        /* Start new word with remaining low bits */
        bw->data = value << (32 - second);
        bw->bits_left = 32 - second;
        return;
    }
    /* There is more space than needed: just insert */
    if (bw->bits_left > nbits)
    {
        bw->bits_left -= nbits;
        bw->data |= value << bw->bits_left;
        return;
    }
    /* Exact fit: write and flush */
    /* (bw->bits_left == nbits) */
    bw->data |= value;
    bw_update_data(bw);
    bw->bits_left = 32;
}

/*****************************************************************************/
/*
 * Copy all bits in bw.data to bw.buf.
 * If 'append_unused' is true, append a byte in bw.buf
 * indicating how many bits in prev bw.buf[index] were unused
 */
static void
bw_flush(struct bit_writer *bw)
{
    int used_bits = 32 - bw->bits_left;
    int nbytes = used_bits / 8;
    int partial_bits = used_bits % 8;
    int i;
    int shift_val;
    int shift_last;

    /* Write full bytes */
    for (i = 0, shift_val = 24; i < nbytes; i++, shift_val -= 8)
    {
        bw->buf[bw->index++] = (bw->data >> shift_val) & 0xff;
    }
    /* Write partial byte */
    if (partial_bits)
    {
        shift_last = 24 - (nbytes * 8);
        bw->buf[bw->index++] = (bw->data >> shift_last) & 0xff;
    }
    /* append unused-bit count */
    bw->buf[bw->index++] = partial_bits ? (8 - partial_bits) : 0;
    bw->data = 0;
    bw->bits_left = 32;
}

/*****************************************************************************/
static void
bw_align_to_byte(struct bit_writer *bw)
{
    int used_bits;
    int full_bytes;
    int i;
    int valid_bits;

    used_bits = 32 - bw->bits_left;
    full_bytes = used_bits / 8;
    /* Flush all full bytes */
    for (i = 0; i < full_bytes; i++)
    {
        bw->buf[bw->index++] = bw->data >> 24;
        bw->data <<= 8;
        bw->bits_left += 8;
        if (bw->bits_left > 32)
        {
            bw->bits_left = 32;
        }
    }
    valid_bits = 32 - bw->bits_left;
    /* if already byte aligned, nothing to do */
    if ((valid_bits & 7) == 0)
    {
        bw->data = 0;   /* <-- important cleanup */
        return;
    }
    /* Flush partial byte */
    bw->buf[bw->index++] = bw->data >> 24;
    bw->data <<= 8;
    bw->bits_left += 8;
    if (bw->bits_left > 32)
    {
        bw->bits_left = 32;
    }
    /* Clean accumulator */
    bw->data = 0;
}

struct bulk_rdp8
{
    unsigned int hash_table[HASH_TABLE_WIDTH * BUCKET_DEPTH];
    unsigned char bucket_count[HASH_TABLE_WIDTH];
    unsigned char hist_buf[HIST_BUF_LEN];
    unsigned int hist_index;
    unsigned char *output_buf;  /* contains compressed data */
    unsigned char *output_buf_plus;
    unsigned int buf_len;    /* length of output_buf */
};

/*****************************************************************************/
static void
clear_tables(unsigned int *hash_table,
             unsigned char *bucket_count,
             unsigned char *hist_buf)
{
    memset(hash_table, 0, HASH_TABLE_WIDTH * BUCKET_DEPTH * 4);
    memset(bucket_count, 0, HASH_TABLE_WIDTH);
    memset(hist_buf, 0, HIST_BUF_LEN);
}

/*****************************************************************************/
static void
update_hash_table(unsigned int *hash_table,
                  unsigned char *bucket_count,
                  unsigned char *hist_buf,
                  int start_index,
                  int num_triplets)
{
    unsigned int u32val;
    unsigned short hash;
    int i;
    int j;
    unsigned char *cptr;

    cptr = &(hist_buf[start_index]);
    for (i = 0; i < num_triplets; i++)
    {
        u32val = (cptr[0] << 8) ^ (cptr[1] << 4) ^ cptr[2];
        u32val ^= u32val >> 7;
        u32val *= 0x9e37;
        u32val >>= 16;
        hash = u32val;
        j = bucket_count[hash] % BUCKET_DEPTH;
        hash_table[hash + j * HASH_TABLE_WIDTH] = start_index + i;
        bucket_count[hash]++;
        cptr++;
    }
}

/*****************************************************************************/
/* returns zero when match is found */
static int
find_longest_match(unsigned int *hash_table,
                   unsigned char *bucket_count,
                   unsigned char *hist_buf,
                   unsigned short hash,
                   int src_buf_index,
                   int src_buf_len,
                   int *cp_offset_ptr,
                   int *lom_ptr)
{
    int num_matches;
    unsigned char *hist_buf_ptr;
    unsigned char *src_buf_ptr;
    int cp_offset;
    int lom;
    int saved_cp_offset;
    int saved_lom;
    int i;
    int j;

    saved_cp_offset = 0;
    saved_lom = 0;
    /* Get number of buckets in this hash.
       Caller has ensured there is at least one */
    num_matches = bucket_count[hash] % BUCKET_DEPTH;
    if (num_matches == 0)
    {
        num_matches = 4;
    }
    src_buf_ptr = &(hist_buf[src_buf_index]);
    for (i = 0; i < num_matches; i++)
    {
        hist_buf_ptr = &(hist_buf[hash_table[hash + HASH_TABLE_WIDTH * i]]);
        cp_offset = hash_table[hash + HASH_TABLE_WIDTH * i];
        if ((hist_buf_ptr[0] == src_buf_ptr[0]) &&
            (hist_buf_ptr[1] == src_buf_ptr[1]) &&
            (hist_buf_ptr[2] == src_buf_ptr[2]))
        {
            j = 3;
            lom = 3;
            while (j < src_buf_len)
            {
                if (hist_buf_ptr[j] != src_buf_ptr[j])
                {
                    break;
                }
                lom++;
                j++;
            }
            if (lom == saved_lom)
            {
                /* If LoM is the same, but cp_offset is closer to src_buf,
                   update saved_cp_offset */
                if (cp_offset > saved_cp_offset)
                {
                    saved_cp_offset = cp_offset;
                }
            }
            else if (lom > saved_lom)
            {
                saved_cp_offset = cp_offset;
                saved_lom = lom;
            }
        }
    }
    if (saved_lom)
    {
        *cp_offset_ptr = src_buf_index - saved_cp_offset;
        *lom_ptr = saved_lom;
        return 0;
    }
    return 1;

}

/*****************************************************************************/
static void
insert_unencoded_literals(struct bit_writer *bw, struct token *token_ptr,
                          const unsigned char *buf, int count)
{
    int ctr;

    if (count < 6)
    {
        for (ctr = 0; ctr < count; ctr++)
        {
            token_ptr = &(g_literals[buf[ctr]]);
            bw_put_bits(bw, token_ptr->code, token_ptr->code_bits);
        }
    }
    else
    {
        /* match distance of zero (10001 00000) is a speical case used to
           indicate start of unencoded literals. The next 15 bits indicate
           count of unencoded literals to follow */
        bw_put_bits(bw, 0x220, 10);
        bw_put_bits(bw, count, 15);
        /* unecoded literals *must* start on a byte boundary */
        bw_align_to_byte(bw);
        /* copy enencoded literals as is to output buffer */
        memcpy(&(bw->buf[bw->index]), buf, count);
        bw->index += count;
    }
}

/*****************************************************************************/
static struct token *
get_dist_token(int dist)
{
    if (dist < 32)      { return &(g_dist_tokens[0]); }
    if (dist < 160)     { return &(g_dist_tokens[1]); }
    if (dist < 672)     { return &(g_dist_tokens[2]); }
    if (dist < 1696)    { return &(g_dist_tokens[3]); }
    if (dist < 5792)    { return &(g_dist_tokens[4]); }
    if (dist < 22176)   { return &(g_dist_tokens[5]); }
    if (dist < 54944)   { return &(g_dist_tokens[6]); }
    if (dist < 317088)  { return &(g_dist_tokens[7]); }
    if (dist < 1365664) { return &(g_dist_tokens[8]); }
    if (dist < 1365664) { return &(g_dist_tokens[8]); }
    if (dist < 2414240) { return &(g_dist_tokens[9]); }
    return &(g_dist_tokens[10]);
}

/*****************************************************************************/
static struct token *
get_lom_token(int lom)
{
    if (lom < 4)        { return &(g_lom_tokens[0]); }
    if (lom < 8)        { return &(g_lom_tokens[1]); }
    if (lom < 16)       { return &(g_lom_tokens[2]); }
    if (lom < 32)       { return &(g_lom_tokens[3]); }
    if (lom < 64)       { return &(g_lom_tokens[4]); }
    if (lom < 128)      { return &(g_lom_tokens[5]); }
    if (lom < 256)      { return &(g_lom_tokens[6]); }
    if (lom < 512)      { return &(g_lom_tokens[7]); }
    if (lom < 1024)     { return &(g_lom_tokens[8]); }
    if (lom < 2048)     { return &(g_lom_tokens[9]); }
    if (lom < 4096)     { return &(g_lom_tokens[10]); }
    if (lom < 8192)     { return &(g_lom_tokens[11]); }
    if (lom < 16384)    { return &(g_lom_tokens[12]); }
    if (lom < 32768)    { return &(g_lom_tokens[13]); }
    return &(g_lom_tokens[14]);
}

/*****************************************************************************/
void *
rdp8_compress_create(int flags)
{
    struct bulk_rdp8 *bulk;

    if ((flags & BULK_PACKET_COMPR_TYPE_RDP8) == 0)
    {
        return NULL;
    }
    bulk = (struct bulk_rdp8 *) calloc(sizeof(struct bulk_rdp8), 1);
    if (bulk == NULL)
    {
        return NULL;
    }
    bulk->buf_len = 64 * 1024;
    bulk->output_buf_plus = (unsigned char *) calloc(bulk->buf_len + 64, 1);
    if (bulk->output_buf_plus == NULL)
    {
        free(bulk);
        return NULL;
    }
    bulk->output_buf = bulk->output_buf_plus + 64;
    return bulk;
}

/*****************************************************************************/
int
rdp8_compress_destroy(void *handle)
{
    struct bulk_rdp8 *bulk;

    bulk = (struct bulk_rdp8 *) handle;
    if (bulk == NULL)
    {
        return 0;
    }
    free(bulk->output_buf_plus);
    free(bulk);
    return 0;
}

/*****************************************************************************/
int
rdp8_compress(void *handle, char **cdata, int *cdata_bytes, int *flags,
              const char *data, int data_bytes)
{
    struct bulk_rdp8 *bulk;
    struct bit_writer bw;
    struct token *token_ptr;
    unsigned int u32val;
    unsigned short hash;
    int bytes_in_seg;
    int hist_start;
    int cp_offset;
    int lom;
    int i;
    int j;
    int no_match_index; /* index in hist_buf where first no match occurred */
    int no_match_count; /* number of bytes that did not match               */

    /* so far, nothing has been compressed */
    if ((cdata == NULL) || (cdata_bytes == NULL) || (flags == NULL))
    {
        return RDP8_ERROR_PARAM;
    }
    if ((data == NULL) || (data_bytes < 1))
    {
        return RDP8_ERROR_PARAM;
    }

    bulk = (struct bulk_rdp8 *) handle;
    bytes_in_seg = data_bytes;

    no_match_index = 0;
    no_match_count = 0;

    if ((*flags & BULK_PACKET_FLUSHED) ||
        ((bulk->hist_index + bytes_in_seg) >= HIST_BUF_LEN))
    {
        clear_tables(bulk->hash_table, bulk->bucket_count, bulk->hist_buf);
        bulk->hist_index = 0;
    }

    hist_start = bulk->hist_index;

    /* copy source data to hist buf at current position */
    memcpy(&(bulk->hist_buf[hist_start]), data, bytes_in_seg);

    bw_init(&bw, bulk->output_buf);

    /* the first two bytes cannot be compressed, output them as literals */
    token_ptr = &(g_literals[bulk->hist_buf[hist_start]]);
    bw_put_bits(&bw, token_ptr->code, token_ptr->code_bits);

    token_ptr = &(g_literals[bulk->hist_buf[hist_start + 1]]);
    bw_put_bits(&bw, token_ptr->code, token_ptr->code_bits);

    /* create hash for first two triplets in history buffer */
    update_hash_table(bulk->hash_table, bulk->bucket_count,
                      bulk->hist_buf, hist_start, 2);

    /* start looking for a match */
    for (i = hist_start + 2; i < hist_start + bytes_in_seg - 2; i++)
    {
        /* compute hash for current triplet */
        u32val = (bulk->hist_buf[i] << 8) ^
                 (bulk->hist_buf[i + 1] << 4) ^
                  bulk->hist_buf[i + 2];
        u32val ^= u32val >> 7;
        u32val *= 0x9e37;
        u32val >>= 16;
        hash = u32val;
        if (bulk->bucket_count[hash] != 0)
        {
            if (find_longest_match(bulk->hash_table, bulk->bucket_count,
                                   bulk->hist_buf, hash, i,
                                   hist_start + bytes_in_seg - i,
                                   &cp_offset, &lom) != 0)
            {
                /* did not find a match; track index and count of no match */
                /* Limit unencoded literals to MAX_UNENCODED_LITERALS */
                if (no_match_count > MAX_UNENCODED_LITERALS)
                {
                    insert_unencoded_literals(&bw, token_ptr,
                                              &(bulk->hist_buf[no_match_index]),
                                              no_match_count);
                    no_match_count = 0;
                    no_match_index = 0;
                }
            }
            else
            {
                /* found a match */
                /* save current hash */
                j = bulk->bucket_count[hash] % BUCKET_DEPTH;
                bulk->hash_table[hash + HASH_TABLE_WIDTH * j] = i;
                bulk->bucket_count[hash]++;
                /* Write previous 'no matches' to output buffer. If count is
                   less than 6,
                   write as encoded literals, else as a string of unencoded
                   literals */
                if (no_match_count)
                {
                    insert_unencoded_literals(&bw, token_ptr,
                                              &(bulk->hist_buf[no_match_index]),
                                              no_match_count);
                    no_match_count = 0;
                    no_match_index = 0;
                }
                /* write match-distance to output buffer */
                token_ptr = get_dist_token(cp_offset);
                bw_put_bits(&bw, token_ptr->code, token_ptr->code_bits);
                bw_put_bits(&bw, cp_offset - token_ptr->value_base,
                            token_ptr->value_bits);
                /* write length-of-match to output buffer */
                token_ptr = get_lom_token(lom);
                bw_put_bits(&bw, token_ptr->code, token_ptr->code_bits);
                bw_put_bits(&bw, lom - token_ptr->value_base,
                            token_ptr->value_bits);
                /* Save hash for all triplets we are skipping. We have already
                   saved hash for first triplet where match occurred. */
                update_hash_table(bulk->hash_table, bulk->bucket_count,
                                  bulk->hist_buf, i + 1, lom - 1);
                i += lom - 1; // -1 because for loop also increments once
                continue;
            }
        } /* if (bucket_count[hash]) */
        /* did not find a match; track index and count of 'no match' */
        if (no_match_index == 0)
        {
            no_match_index = i;
        }
        no_match_count++;
        /* Limit unencoded literals to MAX_UNENCODED_LITERALS */
        if (no_match_count > MAX_UNENCODED_LITERALS)
        {
            insert_unencoded_literals(&bw, token_ptr,
                                      &(bulk->hist_buf[no_match_index]),
                                      no_match_count);
            no_match_count = 0;
            no_match_index = 0;
        }
        /* save hash */
        j = bulk->bucket_count[hash] % BUCKET_DEPTH;
        bulk->hash_table[hash + HASH_TABLE_WIDTH * j] = i;
        bulk->bucket_count[hash]++;
    } /* for */
    /* Write previous 'no matches' to output buffer. If count is less than 6,
       write as encoded literals, else as a string of unencoded literals */
    if (no_match_count)
    {
        insert_unencoded_literals(&bw, token_ptr,
                                  &(bulk->hist_buf[no_match_index]),
                                  no_match_count);
        no_match_count = 0;
        no_match_index = 0;
    }
    /* handle last two bytes */
    while (i < hist_start + bytes_in_seg)
    {
        token_ptr = &(g_literals[bulk->hist_buf[i++]]);
        bw_put_bits(&bw, token_ptr->code, token_ptr->code_bits);
    }
    bw_flush(&bw);
    bulk->hist_index += bytes_in_seg;
    if (data_bytes <= bw.index)
    {
        return RDP8_ERROR_NO_COMPRESS;
    }
    *cdata = (char *) (bulk->output_buf);
    *cdata_bytes = bw.index;
    *flags = BULK_PACKET_COMPR_TYPE_RDP8;
    return RDP8_ERROR_NONE;
}

