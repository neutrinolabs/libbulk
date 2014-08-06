/**
 * MPPC bulk compressor
 *
 * Copyright 2014 Jay Sorg <jay.sorg@gmail.com>
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

/******************************************************************************/
void *
mppc_compress_create(int flags)
{
    return self;
}

/******************************************************************************/
int
mppc_compress_destroy(void *handle)
{
    return 0;
}

/******************************************************************************/
int
mppc_compress(void *handle, char *cdata, int *cdata_bytes,
              char *data, int data_bytes)
{
    return 0;
}
