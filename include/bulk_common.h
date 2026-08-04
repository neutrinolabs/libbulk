/**
 * MPPC bulk compressor / decompressor
 *
 * Copyright 2014-2026 Jay Sorg <jay.sorg@gmail.com>
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

#ifndef __BULK_COMMON_H
#define __BULK_COMMON_H

/* BULK_PACKET_COMPR_TYPE_8K and BULK_PACKET_COMPR_TYPE_64K are used for
   mppc_compress_create
   used in flags for mppc_compress
   BULK_PACKET_COMPR_TYPE_RDP8 used for rdp8_compress_create */
#define BULK_PACKET_COMPRESSED          0x20
#define BULK_PACKET_AT_FRONT            0x40
#define BULK_PACKET_FLUSHED             0x80
#define BULK_PACKET_COMPR_TYPE_8K       0x00
#define BULK_PACKET_COMPR_TYPE_64K      0x01
#define BULK_PACKET_COMPR_TYPE_RDP6     0x02
#define BULK_PACKET_COMPR_TYPE_RDP61    0x03
#define BULK_PACKET_COMPR_TYPE_RDP8     0x04
#define BULK_COMPRESSION_TYPE_MASK      0x0F

/* RDP8 Segmented Data descriptor values */
#define BULK_SEGMENTED_SINGLE       0xE0
#define BULK_SEGMENTED_MULTIPART    0xE1

#endif