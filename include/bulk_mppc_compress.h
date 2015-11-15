/**
 * MPPC bulk compressor
 *
 * Copyright 2014-2015 Jay Sorg <jay.sorg@gmail.com>
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

#ifndef __NL_BULK_MPPC_COMPRESS_H
#define __NL_BULK_MPPC_COMPRESS_H

/* protocol_type for mppc_compress_create */
#define NL_MPPC_FLAGS_RDP40 0x00
#define NL_MPPC_FLAGS_RDP50 0x01
#define NL_MPPC_FLAGS_RDP60 0x02
#define NL_MPPC_FLAGS_RDP61 0x03

/* flags for mppc_compress */
#define NL_PACKET_COMPRESSED       0x20
#define NL_PACKET_AT_FRONT         0x40
#define NL_PACKET_FLUSHED          0x80
#define NL_PACKET_COMPR_TYPE_8K    0x00
#define NL_PACKET_COMPR_TYPE_64K   0x01
#define NL_PACKET_COMPR_TYPE_RDP6  0x02
#define NL_PACKET_COMPR_TYPE_RDP61 0x03
#define NL_COMPRESSION_TYPE_MASK   0x0F

void *
mppc_compress_create(int protocol_type);
int
mppc_compress_destroy(void *handle);
int
mppc_compress(void *handle, char **cdata, int *cdata_bytes, int *flags,
              const char *data, int data_bytes);

#endif
