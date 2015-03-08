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

#ifndef __BULK_RDP8_COMPRESS_H
#define __BULK_RDP8_COMPRESS_H

void *
rdp8_compress_create(int flags);
int
rdp8_compress_destroy(void *handle);
int
rdp8_compress(void *handle, char **cdata, int *cdata_bytes, int *flags,
              const char *data, int data_bytes);

#endif

