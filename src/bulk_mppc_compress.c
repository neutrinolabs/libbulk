/**
 * MPPC bulk compressor
 *
 * Copyright 2012-2015 Laxmikant Rashinkar <LK.Rashinkar@gmail.com>
 * Copyright 2012-2015 Jay Sorg <jay.sorg@gmail.com>
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

#include <bulk_mppc_compress.h>

struct bulk_mppc
{
    int    protocol_type;    /* PROTO_RDP_40, PROTO_RDP_50 etc */
    char  *historyBuffer;    /* contains uncompressed data */
    char  *outputBuffer;     /* contains compressed data */
    char  *outputBufferPlus;
    int    historyOffset;    /* next free slot in historyBuffer */
    int    buf_len;          /* length of historyBuffer, protocol dependant */
    int    bytes_in_opb;     /* compressed bytes available in outputBuffer */
    int    flags;            /* PACKET_COMPRESSED, PACKET_AT_FRONT, PACKET_FLUSHED etc */
    int    flagsHold;
    int    first_pkt;        /* this is the first pkt passing through enc */
    short *hash_table;
};

/******************************************************************************/
/**
 * Initialize bulk_mppc structure
 *
 * @param   protocol_type   PROTO_RDP_40 or PROTO_RDP_50
 *
 * @return  struct xrdp_mppc_enc* or nil on failure
 */
void *
mppc_compress_create(int flags)
{
    struct bulk_mppc *self;

    self = malloc(sizeof(struct bulk_mppc));
    if (self == NULL)
    {
        return NULL;
    }
    memset(self, 0, sizeof(struct bulk_mppc));
    return self; 
}

/******************************************************************************/
int
mppc_compress_destroy(void *handle)
{
    struct bulk_mppc *self;

    self = (struct bulk_mppc *) handle;
    if (self == NULL)
    {
        return 0; 
    }
    free(self);
}

/*****************************************************************************
                     insert 2 bits into outputBuffer
******************************************************************************/
#define insert_2_bits(_data) \
do \
{ \
    if ((bits_left >= 3) && (bits_left <= 8)) \
    { \
        i = bits_left - 2; \
        outputBuffer[opb_index] |= _data << i; \
        bits_left = i; \
    } \
    else \
    { \
        i = 2 - bits_left; \
        j = 8 - i; \
        outputBuffer[opb_index++] |= _data >> i; \
        outputBuffer[opb_index] |= _data << j; \
        bits_left = j; \
    } \
} while (0)

/*****************************************************************************
                     insert 3 bits into outputBuffer
******************************************************************************/
#define insert_3_bits(_data) \
do \
{ \
    if ((bits_left >= 4) && (bits_left <= 8)) \
    { \
        i = bits_left - 3; \
        outputBuffer[opb_index] |= _data << i; \
        bits_left = i; \
    } \
    else \
    { \
        i = 3 - bits_left; \
        j = 8 - i; \
        outputBuffer[opb_index++] |= _data >> i; \
        outputBuffer[opb_index] |= _data << j; \
        bits_left = j; \
    } \
} while (0)

/*****************************************************************************
                     insert 4 bits into outputBuffer
******************************************************************************/
#define insert_4_bits(_data) \
do \
{ \
    if ((bits_left >= 5) && (bits_left <= 8)) \
    { \
        i = bits_left - 4; \
        outputBuffer[opb_index] |= _data << i; \
        bits_left = i; \
    } \
    else \
    { \
        i = 4 - bits_left; \
        j = 8 - i; \
        outputBuffer[opb_index++] |= _data >> i; \
        outputBuffer[opb_index] |= _data << j; \
        bits_left = j; \
    } \
} while (0)

/*****************************************************************************
                     insert 5 bits into outputBuffer
******************************************************************************/
#define insert_5_bits(_data) \
do \
{ \
    if ((bits_left >= 6) && (bits_left <= 8)) \
    { \
        i = bits_left - 5; \
        outputBuffer[opb_index] |= _data << i; \
        bits_left = i; \
    } \
    else \
    { \
        i = 5 - bits_left; \
        j = 8 - i; \
        outputBuffer[opb_index++] |= _data >> i; \
        outputBuffer[opb_index] |= _data << j; \
        bits_left = j; \
    } \
} while (0)

/*****************************************************************************
                     insert 6 bits into outputBuffer
******************************************************************************/
#define insert_6_bits(_data) \
do \
{ \
    if ((bits_left >= 7) && (bits_left <= 8)) \
    { \
        i = bits_left - 6; \
        outputBuffer[opb_index] |= (_data << i); \
        bits_left = i; \
    } \
    else \
    { \
        i = 6 - bits_left; \
        j = 8 - i; \
        outputBuffer[opb_index++] |= (_data >> i); \
        outputBuffer[opb_index] |= (_data << j); \
        bits_left = j; \
    } \
} while (0)

/*****************************************************************************
                     insert 7 bits into outputBuffer
******************************************************************************/
#define insert_7_bits(_data) \
do \
{ \
    if (bits_left == 8) \
    { \
        outputBuffer[opb_index] |= _data << 1; \
        bits_left = 1; \
    } \
    else \
    { \
        i = 7 - bits_left; \
        j = 8 - i; \
        outputBuffer[opb_index++] |= _data >> i; \
        outputBuffer[opb_index] |= _data << j; \
        bits_left = j; \
    } \
} while (0)

/*****************************************************************************
                     insert 8 bits into outputBuffer
******************************************************************************/
#define insert_8_bits(_data) \
do \
{ \
    if (bits_left == 8) \
    { \
        outputBuffer[opb_index++] |= _data; \
        bits_left = 8; \
    } \
    else \
    { \
        i = 8 - bits_left; \
        j = 8 - i; \
        outputBuffer[opb_index++] |= _data >> i; \
        outputBuffer[opb_index] |= _data << j; \
        bits_left = j; \
    } \
} while (0)

/*****************************************************************************
                     insert 9 bits into outputBuffer
******************************************************************************/
#define insert_9_bits(_data16) \
do \
{ \
    i = 9 - bits_left; \
    j = 8 - i; \
    outputBuffer[opb_index++] |= (char) (_data16 >> i); \
    outputBuffer[opb_index] |= (char) (_data16 << j); \
    bits_left = j; \
    if (bits_left == 0) \
    { \
        opb_index++; \
        bits_left = 8; \
    } \
} while (0)

/*****************************************************************************
                     insert 10 bits into outputBuffer
******************************************************************************/
#define insert_10_bits(_data16) \
do \
{ \
    i = 10 - bits_left; \
    if ((bits_left >= 3) && (bits_left <= 8)) \
    { \
        j = 8 - i; \
        outputBuffer[opb_index++] |= (char) (_data16 >> i); \
        outputBuffer[opb_index] |= (char) (_data16 << j); \
        bits_left = j; \
    } \
    else \
    { \
        j = i - 8; \
        k = 8 - j; \
        outputBuffer[opb_index++] |= (char) (_data16 >> i); \
        outputBuffer[opb_index++] |= (char) (_data16 >> j); \
        outputBuffer[opb_index] |= (char) (_data16 << k); \
        bits_left = k; \
    } \
} while (0)

/*****************************************************************************
                     insert 11 bits into outputBuffer
******************************************************************************/
#define insert_11_bits(_data16) \
do \
{ \
    i = 11 - bits_left; \
    if ((bits_left >= 4) && (bits_left <= 8)) \
    { \
        j = 8 - i; \
        outputBuffer[opb_index++] |= (char) (_data16 >> i); \
        outputBuffer[opb_index] |= (char) (_data16 << j); \
        bits_left = j; \
    } \
    else \
    { \
        j = i - 8;                                \
        k = 8 - j; \
        outputBuffer[opb_index++] |= (char) (_data16 >> i); \
        outputBuffer[opb_index++] |= (char) (_data16 >> j); \
        outputBuffer[opb_index] |= (char) (_data16 << k); \
        bits_left = k; \
    } \
} while (0)

/*****************************************************************************
                     insert 12 bits into outputBuffer
******************************************************************************/
#define insert_12_bits(_data16) \
do \
{ \
    i = 12 - bits_left; \
    if ((bits_left >= 5) && (bits_left <= 8)) \
    { \
        j = 8 - i; \
        outputBuffer[opb_index++] |= (char) (_data16 >> i); \
        outputBuffer[opb_index] |= (char) (_data16 << j); \
        bits_left = j; \
    } \
    else \
    { \
        j = i - 8; \
        k = 8 - j; \
        outputBuffer[opb_index++] |= (char) (_data16 >> i); \
        outputBuffer[opb_index++] |= (char) (_data16 >> j); \
        outputBuffer[opb_index] |= (char) (_data16 << k); \
        bits_left = k; \
    } \
} while (0)

/*****************************************************************************
                     insert 13 bits into outputBuffer
******************************************************************************/
#define insert_13_bits(_data16) \
do \
{ \
    i = 13 - bits_left; \
    if ((bits_left >= 6) && (bits_left <= 8)) \
    { \
        j = 8 - i; \
        outputBuffer[opb_index++] |= (char) (_data16 >> i); \
        outputBuffer[opb_index] |= (char) (_data16 << j); \
        bits_left = j; \
    } \
    else \
    { \
        j = i - 8; \
        k = 8 - j; \
        outputBuffer[opb_index++] |= (char) (_data16 >> i); \
        outputBuffer[opb_index++] |= (char) (_data16 >> j); \
        outputBuffer[opb_index] |= (char) (_data16 << k); \
        bits_left = k; \
    } \
} while (0)

/*****************************************************************************
                     insert 14 bits into outputBuffer
******************************************************************************/
#define insert_14_bits(_data16) \
do \
{ \
    i = 14 - bits_left; \
    if ((bits_left >= 7) && (bits_left <= 8)) \
    { \
        j = 8 - i; \
        outputBuffer[opb_index++] |= (char) (_data16 >> i); \
        outputBuffer[opb_index] |= (char) (_data16 << j); \
        bits_left = j; \
    } \
    else \
    { \
        j = i - 8; \
        k = 8 - j; \
        outputBuffer[opb_index++] |= (char) (_data16 >> i); \
        outputBuffer[opb_index++] |= (char) (_data16 >> j); \
        outputBuffer[opb_index] |= (char) (_data16 << k); \
        bits_left = k; \
    } \
} while (0)

/*****************************************************************************
                     insert 15 bits into outputBuffer
******************************************************************************/
#define insert_15_bits(_data16) \
do \
{ \
    i = 15 - bits_left; \
    if (bits_left == 8) \
    { \
        j = 8 - i; \
        outputBuffer[opb_index++] |= (char) (_data16 >> i); \
        outputBuffer[opb_index] |= (char) (_data16 << j); \
        bits_left = j; \
    } \
    else \
    { \
        j = i - 8; \
        k = 8 - j; \
        outputBuffer[opb_index++] |= (char) (_data16 >> i); \
        outputBuffer[opb_index++] |= (char) (_data16 >> j); \
        outputBuffer[opb_index] |= (char) (_data16 << k); \
        bits_left = k; \
    } \
} while (0)

/*****************************************************************************
                     insert 16 bits into outputBuffer
******************************************************************************/
#define insert_16_bits(_data16) \
do \
{ \
    i = 16 - bits_left; \
    j = i - 8; \
    k = 8 - j; \
    outputBuffer[opb_index++] |= (char) (_data16 >> i); \
    outputBuffer[opb_index++] |= (char) (_data16 >> j); \
    outputBuffer[opb_index] |= (char) (_data16 << k); \
    bits_left = k; \
} while (0)

/******************************************************************************/
int
mppc_compress(void *handle, char **cdata, int *cdata_bytes, int *flags,
              const char *data, int data_bytes)
{
    struct bulk_mppc *self;

    self = (struct bulk_mppc *) handle;
    return 0;
}
