/**
 * MPPC bulk compressor
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

#ifndef __BULK_MPPC_COMPRESS_H
#define __BULK_MPPC_COMPRESS_H

#include <bulk_common.h>

#define MPPC_ERROR_NONE         0
#define MPPC_ERROR_NO_COMPRESS  1
#define MPPC_ERROR_PARAM        2
#define MPPC_ERROR_NOIMP        3
#define MPPC_ERROR_OTHER        16

/**
 * Creates an encoder object
 *
 * @param protocol_type One of BULK_PACKET_COMPR_TYPE_*
 * @return Pointer to an encoder object or NULL
 *
 * The return pointer can be used in later calls to mppc_compress_destroy
 * and mppc_compress
 */
void *
mppc_compress_create(int protocol_type);

/**
 * Deletes an encoder object
 *
 * @param handle Pointer to an encoder object or NULL
 * @return Always returns MPPC_ERROR_NONE
 */
int
mppc_compress_destroy(void *handle);

/**
 * Compress a PDU
 *
 * @param handle Pointer to an encoder object or NULL
 * @param cdata Pointer to a pointer that received the address of the
 *              compressed data.  The data has a 64 bytes empty header
 *              preceding this pointer that can be used by the application
 * @param cdata_bytes Pointer to an integer that receives the compressed
 *                    data size
 * @param flags Pointer to an integer that receives the compression flags
 * @param data Pointer to the data to compress
 * @param data_bytes The number of bytes to compress
 * @return Returns MPPC_ERROR_NONE on successful compression else
 *         one of MPPC_ERROR_*
 */
int
mppc_compress(void *handle, void **cdata, int *cdata_bytes, int *flags,
              const void *data, int data_bytes);

#endif
