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

#ifndef __BULK_COMMON_PRIVATE_H
#define __BULK_COMMON_PRIVATE_H

#define BULK_MIN(_x, _y) ((_x) < (_y) ? (_x) : (_y))

#define BULK_PUT_U16_LE(_buf, _idx, _val)                   \
    do {                                                    \
        (_buf)[(_idx) + 0] = (unsigned char)((_val) >> 0);  \
        (_buf)[(_idx) + 1] = (unsigned char)((_val) >> 8);  \
    } while (0)

#define BULK_PUT_U32_LE(_buf, _idx, _val)                   \
    do {                                                    \
        (_buf)[(_idx) + 0] = (unsigned char)((_val) >> 0);  \
        (_buf)[(_idx) + 1] = (unsigned char)((_val) >> 8);  \
        (_buf)[(_idx) + 2] = (unsigned char)((_val) >> 16); \
        (_buf)[(_idx) + 3] = (unsigned char)((_val) >> 24); \
    } while (0)

#define BULK_PUT_U16_BE(_buf, _idx, _val)                   \
    do {                                                    \
        (_buf)[(_idx) + 0] = (unsigned char)((_val) >> 8);  \
        (_buf)[(_idx) + 1] = (unsigned char)((_val) >> 0);  \
    } while (0)

#define BULK_PUT_U32_BE(_buf, _idx, _val)                   \
    do {                                                    \
        (_buf)[(_idx) + 0] = (unsigned char)((_val) >> 24); \
        (_buf)[(_idx) + 1] = (unsigned char)((_val) >> 16); \
        (_buf)[(_idx) + 2] = (unsigned char)((_val) >> 8);  \
        (_buf)[(_idx) + 3] = (unsigned char)((_val) >> 0);  \
    } while (0)

#endif
