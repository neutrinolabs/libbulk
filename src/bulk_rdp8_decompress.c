/**
 * RDP8 bulk decompressor
 *
 * Copyright 2015 Jay Sorg <jay.sorg@gmail.com>
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

/* https://msdn.microsoft.com/en-us/library/hh881173.aspx */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bulk_rdp8_decompress.h>
#include "getset.h"

/* flags for rdp8_compress_create */
#define NL_RDP8_FLAGS_RDP80 0x04

/* flags for mppc_compress */
#define NL_PACKET_COMPRESSED       0x20
#define NL_PACKET_COMPR_TYPE_RDP8  0x04
#define NL_COMPRESSION_TYPE_MASK   0x0F

typedef unsigned char byte;
typedef unsigned short uint16;
typedef unsigned int uint32;

/* descriptor values */
#define SEGMENTED_SINGLE        0xE0
#define SEGMENTED_MULTIPART     0xE1

/* token assignments from the spec, sorted by prefixLength */

typedef struct _Token
{
    int prefixLength;     /* number of bits in the prefix */
    int prefixCode;       /* bit pattern of this prefix */
    int valueBits;        /* number of value bits to read */
    int tokenType;        /* 0=literal, 1=match */
    uint32 valueBase;     /* added to the value bits */
} Token;

const Token g_tokenTable[] =
{
    /* len code vbits type  vbase */
    {  1,    0,   8,   0,           0 },    /* 0 */
    {  5,   17,   5,   1,           0 },    /* 10001 */
    {  5,   18,   7,   1,          32 },    /* 10010 */
    {  5,   19,   9,   1,         160 },    /* 10011 */
    {  5,   20,  10,   1,         672 },    /* 10100 */
    {  5,   21,  12,   1,        1696 },    /* 10101 */
    {  5,   24,   0,   0,  0x00       },    /* 11000 */
    {  5,   25,   0,   0,  0x01       },    /* 11001 */
    {  6,   44,  14,   1,        5792 },    /* 101100 */
    {  6,   45,  15,   1,       22176 },    /* 101101 */
    {  6,   52,   0,   0,  0x02       },    /* 110100 */
    {  6,   53,   0,   0,  0x03       },    /* 110101 */
    {  6,   54,   0,   0,  0xFF       },    /* 110110 */
    {  7,   92,  18,   1,       54944 },    /* 1011100 */
    {  7,   93,  20,   1,      317088 },    /* 1011101 */
    {  7,  110,   0,   0,  0x04       },    /* 1101110 */
    {  7,  111,   0,   0,  0x05       },    /* 1101111 */
    {  7,  112,   0,   0,  0x06       },    /* 1110000 */
    {  7,  113,   0,   0,  0x07       },    /* 1110001 */
    {  7,  114,   0,   0,  0x08       },    /* 1110010 */
    {  7,  115,   0,   0,  0x09       },    /* 1110011 */
    {  7,  116,   0,   0,  0x0A       },    /* 1110100 */
    {  7,  117,   0,   0,  0x0B       },    /* 1110101 */
    {  7,  118,   0,   0,  0x3A       },    /* 1110110 */
    {  7,  119,   0,   0,  0x3B       },    /* 1110111 */
    {  7,  120,   0,   0,  0x3C       },    /* 1111000 */
    {  7,  121,   0,   0,  0x3D       },    /* 1111001 */
    {  7,  122,   0,   0,  0x3E       },    /* 1111010 */
    {  7,  123,   0,   0,  0x3F       },    /* 1111011 */
    {  7,  124,   0,   0,  0x40       },    /* 1111100 */
    {  7,  125,   0,   0,  0x80       },    /* 1111101 */
    {  8,  188,  20,   1,     1365664 },    /* 10111100 */
    {  8,  189,  21,   1,     2414240 },    /* 10111101 */
    {  8,  252,   0,   0,  0x0C       },    /* 11111100 */
    {  8,  253,   0,   0,  0x38       },    /* 11111101 */
    {  8,  254,   0,   0,  0x39       },    /* 11111110 */
    {  8,  255,   0,   0,  0x66       },    /* 11111111 */
    {  9,  380,  22,   1,     4511392 },    /* 101111100 */
    {  9,  381,  23,   1,     8705696 },    /* 101111101 */
    {  9,  382,  24,   1,    17094304 },    /* 101111110 */
    { 0 }
};

struct bulk_rdp8
{
    const byte *m_pbInputCurrent;     /* ptr into input bytes */
    const byte *m_pbInputEnd;         /* ptr past end of input */

    /* input bit stream */
    uint32 m_cBitsRemaining;          /* # bits input remaining8 */
    uint32 m_BitsCurrent;             /* remainder of most-recent byte */
    uint32 m_cBitsCurrent;            /* number of bits in m_BitsCurrent */

    /* decompressed output */
    byte m_outputBuffer[65536];       /* most-recent Decompress result */
    uint32 m_outputCount;             /* length in m_outputBuffer */

    /* decompression history */
    byte m_historyBuffer[2500000];    /* last N bytes of output */
    uint32 m_historyIndex;            /* index for next byte out */
};

/*****************************************************************************/
void *
rdp8_decompress_create(int flags)
{
    struct bulk_rdp8 *bulk;

    if ((flags & NL_RDP8_FLAGS_RDP80) == 0)
    {
        return NULL;
    }
    bulk = (struct bulk_rdp8 *) malloc(sizeof(struct bulk_rdp8));
    if (bulk == NULL)
    {
        return NULL;
    }
    memset(bulk, 0, sizeof(struct bulk_rdp8));
    return bulk;
}

/*****************************************************************************/
int
rdp8_decompress_destroy(void *handle)
{
    struct bulk_rdp8 *bulk;

    bulk = (struct bulk_rdp8 *) handle;
    if (bulk == NULL)
    {
        return 1;
    }
    free(bulk);
    return 0;
}

/*****************************************************************************/
/*  Return the value of the next 'bitCount' bits as unsigned. */
static uint32
GetBits(struct bulk_rdp8 *bulk, uint32 bitCount)
{
    uint32 result;

    while (bulk->m_cBitsCurrent < bitCount)
    {
        bulk->m_BitsCurrent <<= 8;
        if (bulk->m_pbInputCurrent < bulk->m_pbInputEnd)
        {
            bulk->m_BitsCurrent += *(bulk->m_pbInputCurrent++);
        }
        bulk->m_cBitsCurrent += 8;
    }

    bulk->m_cBitsRemaining -= bitCount;
    bulk->m_cBitsCurrent -= bitCount;
    result = bulk->m_BitsCurrent >> bulk->m_cBitsCurrent;
    bulk->m_BitsCurrent &= ((1 << bulk->m_cBitsCurrent) - 1);
    return result;
}

/*****************************************************************************/
static int
OutputFromCompressed(struct bulk_rdp8 *bulk, const byte *pbEncoded,
                     int cbEncoded)
{
    int haveBits;
    int inPrefix;
    byte c;
    uint32 count;
    uint32 distance;
    int opIndex;
    int extra;
    uint32 prevIndex;

    bulk->m_outputCount = 0;
    bulk->m_pbInputCurrent = pbEncoded;
    bulk->m_pbInputEnd = pbEncoded + cbEncoded - 1;

    bulk->m_cBitsRemaining = 8 * (cbEncoded - 1) - *(bulk->m_pbInputEnd);
    bulk->m_cBitsCurrent = 0;
    bulk->m_BitsCurrent = 0;

    while (bulk->m_cBitsRemaining)
    {
        haveBits = 0;
        inPrefix = 0;

        /* Scan the token table, considering more bits as needed,
           until the resulting token is found. */

        for (opIndex = 0; g_tokenTable[opIndex].prefixLength != 0; opIndex++)
        {
            /* get more bits if needed */
            while (haveBits < g_tokenTable[opIndex].prefixLength)
            {
                inPrefix = (inPrefix << 1) + GetBits(bulk, 1);
                haveBits++;
            }

            if (inPrefix == g_tokenTable[opIndex].prefixCode)
            {
                if (g_tokenTable[opIndex].tokenType == 0)
                {
                    c = (byte)(g_tokenTable[opIndex].valueBase +
                        GetBits(bulk, g_tokenTable[opIndex].valueBits));
                    goto output_literal;
                }
                else
                {
                    distance = g_tokenTable[opIndex].valueBase +
                               GetBits(bulk, g_tokenTable[opIndex].valueBits);
                    if (distance != 0)
                    {
                        if (GetBits(bulk, 1) == 0)
                        {
                            count = 3;
                        }
                        else
                        {
                            count = 4;
                            extra = 2;
                            while (GetBits(bulk, 1) == 1)
                            {
                                count *= 2;
                                extra++;
                            }

                            count += GetBits(bulk, extra);
                        }
                        goto output_match;
                    }
                    else /* match distance == 0 is special case */
                    {
                        count = GetBits(bulk, 15);

                        /* discard remaining bits */
                        bulk->m_cBitsRemaining -= bulk->m_cBitsCurrent;
                        bulk->m_cBitsCurrent = 0;
                        bulk->m_BitsCurrent = 0;
                        goto output_unencoded;
                    }
                }
            }
        }
        break;

output_literal:

        /* Add one byte 'c' to output and history */
        bulk->m_historyBuffer[bulk->m_historyIndex] = c;
        if (++(bulk->m_historyIndex) == sizeof(bulk->m_historyBuffer))
        {
            bulk->m_historyIndex = 0;
        }
        bulk->m_outputBuffer[bulk->m_outputCount++] = c;
        continue;

output_match:

        /* Add 'count' bytes from 'distance' back in history. */
        /* Output these bytes again, and add to history again. */
        prevIndex = bulk->m_historyIndex + 
                    sizeof(bulk->m_historyBuffer) - distance;
        prevIndex = prevIndex % sizeof(bulk->m_historyBuffer);

        /* n.b. memcpy or movsd, for example, will not work here. */
        /* Overlapping matches must replicate.  movsb might work. */
        while (count--)
        {
            c = bulk->m_historyBuffer[prevIndex];
            if (++prevIndex == sizeof(bulk->m_historyBuffer))
            {
                prevIndex = 0;
            }

            bulk->m_historyBuffer[bulk->m_historyIndex] = c;
            if (++(bulk->m_historyIndex) == sizeof(bulk->m_historyBuffer))
            {
                bulk->m_historyIndex = 0;
            }

            bulk->m_outputBuffer[bulk->m_outputCount] = c;
            ++(bulk->m_outputCount);
        }
        continue;

output_unencoded:

        /* Copy 'count' bytes from stream input to output */
        /* and add to history. */
        while (count--)
        {
            c = *(bulk->m_pbInputCurrent++);
            bulk->m_cBitsRemaining -= 8;

            bulk->m_historyBuffer[bulk->m_historyIndex] = c;
            if (++(bulk->m_historyIndex) == sizeof(bulk->m_historyBuffer))
            {
                bulk->m_historyIndex = 0;
            }

            bulk->m_outputBuffer[bulk->m_outputCount] = c;
            ++(bulk->m_outputCount);
        }
        continue;
    }
    return 0;
}

/*****************************************************************************/
static int
OutputFromNotCompressed(struct bulk_rdp8 *bulk, const byte *pbRaw, int cbRaw)
{
    int iRaw;
    byte c;

    bulk->m_outputCount = 0;
    for (iRaw = 0; iRaw < cbRaw; iRaw++)
    {
        c = pbRaw[iRaw];
        bulk->m_historyBuffer[bulk->m_historyIndex++] = c;
        if (bulk->m_historyIndex == sizeof(bulk->m_historyBuffer))
        {
            bulk->m_historyIndex = 0;
        }
        bulk->m_outputBuffer[bulk->m_outputCount++] = c;
    }
    return 0;
}

/*****************************************************************************/
static int
OutputFromSegment(struct bulk_rdp8 *bulk, const byte *pbSegment,
                  int cbSegment)
{
    if (pbSegment[0] & NL_PACKET_COMPRESSED)
    {
       return OutputFromCompressed(bulk, pbSegment + 1, cbSegment - 1);
    }
    else
    {
       return OutputFromNotCompressed(bulk, pbSegment + 1, cbSegment - 1);
    }
}

/*****************************************************************************/
int
rdp8_decompress(void *handle, const char *cdata, int cdata_bytes, int flags,
                char **data, int *data_bytes)
{
    struct bulk_rdp8 *bulk;

    byte descriptor;
    uint16 segmentCount;
    uint32 uncompressedSize;

    uint32 segmentOffset;
    byte *pConcatenated;
    uint16 segmentNumber;
    uint32 size;
    const byte *lcdata;

    bulk = (struct bulk_rdp8 *) handle;
    if (bulk == NULL)
    {
        return 1;
    }
    if ((flags & NL_COMPRESSION_TYPE_MASK) != NL_PACKET_COMPR_TYPE_RDP8)
    {
        return 0;
    }
    descriptor = GGET_UINT8(cdata, 0);
    if (descriptor == SEGMENTED_SINGLE)
    {
        lcdata = (const byte *) (cdata + 1);
        if (OutputFromSegment(bulk, lcdata, cdata_bytes - 1) != 0)
        {
            return 1;
        }
        *data = (char *) malloc(bulk->m_outputCount);
        *data_bytes = bulk->m_outputCount;
        memcpy(*data, bulk->m_outputBuffer, bulk->m_outputCount);
    }
    else if (descriptor == SEGMENTED_MULTIPART)
    {
        segmentCount = GGET_UINT16(cdata, 1);
        uncompressedSize = GGET_UINT32(cdata, 3);
        segmentOffset = 7;
        pConcatenated = (byte *) malloc(uncompressedSize);
        *data = (char *) pConcatenated;
        *data_bytes = uncompressedSize;
        for (segmentNumber = 0; segmentNumber < segmentCount; segmentNumber++)
        {
            size = GGET_UINT32(cdata, segmentOffset);
            lcdata = (const byte *) (cdata + segmentOffset + 4);
            if (OutputFromSegment(bulk, lcdata, size) != 0)
            {
                return 1;
            }
            segmentOffset += 4 + size;
            memcpy(pConcatenated, bulk->m_outputBuffer, bulk->m_outputCount);
            pConcatenated += bulk->m_outputCount;
        }
    }
    return 0;
}

