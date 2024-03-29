/*
 * Copyright (C) 2018 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

/* Soc-Vendor defined gralloc buffer handle. */
#ifndef GRALLOC_PRIV_H_
#define GRALLOC_PRIV_H_

#include <cutils/native_handle.h>

#include <sys/cdefs.h>
#include <stdint.h>
#include <limits.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>

#include <cutils/native_handle.h>

#ifndef GRALLOC_USAGE_TILED_VIV
#  define GRALLOC_USAGE_TILED_VIV       0x10000000
#endif
#ifndef GRALLOC_USAGE_TS_VIV
#  define GRALLOC_USAGE_TS_VIV          0x20000000
#endif
#ifndef GRALLOC_USAGE_CLEAR_VIV
#  define GRALLOC_USAGE_CLEAR_VIV       0x80000000
#endif

#define DRV_MAX_PLANES                  4
#define BUFFER_NAME_MAX_SIZE            64

struct private_handle_t {
    native_handle_t nativeHandle;

    /* file descriptors */
    int fd;

    /* integers */
    int magic;

    int flags;
    int size;
    int offset;
    uint64_t base __attribute__((aligned(8)));
    uint64_t phys __attribute__((aligned(8)));

    int width;
    int height;
    int format;
    int stride; /* the stride in pixels. */

    int usage;
    int pid;    /* owner of data (for validation) */

    uint64_t fsl_reserved[3] __attribute__((aligned(8)));
    uint64_t surface;

    /* pointer to some bo struct. */
    uint64_t data __attribute__((aligned(8)));
    uint64_t reserved[3];
};

#define GRALLOC_PRIVATE_HANDLE_MAGIC   0x3141592
#define GRALLOC_PRIVATE_HANDLE_NUM_FDS 1
#define GRALLOC_PRIVATE_HANDLE_NUM_INTS ( \
    ((sizeof(struct private_handle_t) - sizeof(native_handle_t))/sizeof(int)) \
     - GRALLOC_PRIVATE_HANDLE_NUM_FDS)

#endif /* GRALLOC_PRIV_H_ */
