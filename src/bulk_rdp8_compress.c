/**
 * RDP8 bulk compressor
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <bulk_rdp8_compress.h>
#include "getset.h"

/* flags for rdp8_compress_create */
#define NL_RDP8_FLAGS_RDP80 0x04

/* flags for mppc_compress */
#define NL_PACKET_COMPRESSED       0x20
#define NL_PACKET_COMPR_TYPE_RDP8  0x04
#define NL_COMPRESSION_TYPE_MASK   0x0F

struct bulk_rdp8
{
    int i1;
};

/*****************************************************************************/
void *
rdp8_compress_create(int flags)
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
rdp8_compress_destroy(void *handle)
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
int
rdp8_compress(void *handle, char **cdata, int *cdata_bytes, int *flags,
              const char *data, int data_bytes)
{
    return 0;
}

