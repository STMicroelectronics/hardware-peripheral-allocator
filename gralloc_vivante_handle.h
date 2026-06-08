/****************************************************************************
*
*    Copyright (c) 2005 - 2024 by Vivante Corp.  All rights reserved.
*
*    The material in this file is confidential and contains trade secrets
*    of Vivante Corporation. This is proprietary information owned by
*    Vivante Corporation. No part of this work may be disclosed,
*    reproduced, copied, transmitted, or used in any way for any purpose,
*    without the express written permission of Vivante Corporation.
*
*****************************************************************************/

#ifndef GRALLOC_HANDLE_H_
#define GRALLOC_HANDLE_H_

#include "gralloc_priv.h"

#include <cutils/native_handle.h>
#include <log/log.h>

#include <unistd.h>
#include <stdint.h>
#include <errno.h>
#include <string.h>

typedef struct private_handle_t gralloc_handle_t;
typedef struct private_handle_t *gralloc_handle_p;

/* helper */
static inline gralloc_handle_t* gralloc_handle(buffer_handle_t handle)
{
    return (gralloc_handle_t *)handle;
}

/*
 * Following functions are to hide platform specific gralloc handle
 * definition.
 *
 * Vivante drivers will only call these functions, instead of access
 * gralloc handle struct directly.
 */

/* handle creation. */
static inline buffer_handle_t gralloc_handle_create(int width, int height,
                                    int format, uint64_t usage)
{
    gralloc_handle_t *hnd;
    const int numFds = GRALLOC_PRIVATE_HANDLE_NUM_FDS;
    const int numInts = GRALLOC_PRIVATE_HANDLE_NUM_INTS;

    hnd = (gralloc_handle_t *)native_handle_create(numFds, numInts);
    if (!hnd)
        return NULL;

    memset(hnd->nativeHandle.data, 0, (numFds + numInts) * sizeof(int));
    hnd->fd = -1;
    hnd->magic = GRALLOC_PRIVATE_HANDLE_MAGIC;

    hnd->width = width;
    hnd->height = height;
    hnd->format = format;
    hnd->stride = 0;
    hnd->usage = usage;
    hnd->pid = 0;
    hnd->data = 0;

    ALOGV("create handle: version=%d, numInts=%d, numFds=%d, magic=%x",
            hnd->nativeHandle.version, hnd->nativeHandle.numInts,
            hnd->nativeHandle.numFds, hnd->magic);

    return (buffer_handle_t)hnd;
}

/* handle destroy. */
static inline void gralloc_handle_free(buffer_handle_t handle)
{
    gralloc_handle_t *hnd = (gralloc_handle_t *)handle;
    hnd->magic = 0;
#ifdef __GNUC__
    asm volatile("":::"memory");
#endif
    native_handle_delete(&hnd->nativeHandle);
}

/* validate, returns error code. */
static inline int gralloc_handle_validate_taged(buffer_handle_t handle,
                        const char *func, int line)
{
    gralloc_handle_t *hnd = (gralloc_handle_t *)handle;

    if (!hnd) {
        ALOGE("%s(%d): invalid null handle", func, line);
        return -EINVAL;
    }

    if (hnd && (hnd->nativeHandle.version != sizeof(hnd->nativeHandle) ||
                hnd->magic != GRALLOC_PRIVATE_HANDLE_MAGIC)) {
        ALOGE("%s(%d): invalid handle: version=%d, numInts=%d, numFds=%d, magic=%x",
              func, line,
              hnd->nativeHandle.version, hnd->nativeHandle.numInts,
              hnd->nativeHandle.numFds, hnd->magic);
        return -EINVAL;
    }
    return 0;
}

#define gralloc_handle_validate(handle) \
    gralloc_handle_validate_taged(handle, __FUNCTION__, __LINE__)

/* get prime fd. */
static inline int gralloc_handle_fd(buffer_handle_t handle)
{
    return ((gralloc_handle_t *)handle)->fd;
}

/* set prime fd. */
static inline void gralloc_handle_set_fd(buffer_handle_t handle, int fd)
{
    ((gralloc_handle_t *)handle)->fd = fd;
}

/* get name */
static inline int gralloc_handle_name(buffer_handle_t handle, char* name)
{
    if (name) {
        strncpy(name, ((gralloc_handle_t *)handle)->name, BUFFER_NAME_MAX_SIZE);
        return 0;
    }
    else {
        return 1;
    }
}

/* get id, id is immutable. */
static inline int gralloc_handle_id(buffer_handle_t handle)
{
    return ((gralloc_handle_t *)handle)->id;
}

/* get width, width is immutable. */
static inline int32_t gralloc_handle_width(buffer_handle_t handle)
{
    return ((gralloc_handle_t *)handle)->width;
}

/* get height, height is immutable. */
static inline int32_t gralloc_handle_height(buffer_handle_t handle)
{
    return ((gralloc_handle_t *)handle)->height;
}

/* get format, format is immutable. */
static inline int32_t gralloc_handle_format(buffer_handle_t handle)
{
    return ((gralloc_handle_t *)handle)->format;
}

/* get format_modifier */
static inline int gralloc_handle_format_modifier(buffer_handle_t handle)
{
    return ((gralloc_handle_t *)handle)->format_modifier;
}

/* get usage, usage is immutable. */
static inline uint64_t gralloc_handle_usage(buffer_handle_t handle)
{
    return ((gralloc_handle_t *)handle)->usage;
}

/* get stride in pixels. */
static inline uint32_t gralloc_handle_stride(buffer_handle_t handle)
{
    return ((gralloc_handle_t *)handle)->stride;
}

/* set stride in pixels. */
static inline void gralloc_handle_set_stride(buffer_handle_t handle, uint32_t stride)
{
    ((gralloc_handle_t *)handle)->stride = stride;
}

/* get data owner, ie the pid. */
static inline int32_t gralloc_handle_data_owner(buffer_handle_t handle)
{
    return ((gralloc_handle_t *)handle)->pid;
}

/* get data, ie the bo. */
static inline void * gralloc_handle_data(buffer_handle_t handle)
{
    return (void *)(uintptr_t)((gralloc_handle_t *)handle)->data;
}

/* set data along with owner. */
static inline void gralloc_handle_set_data(buffer_handle_t handle,
                        void *data, int data_owner)
{
    gralloc_handle_t * hnd = (gralloc_handle_t *)handle;
    hnd->data = (uintptr_t)data;
    hnd->pid = data_owner;
}

static inline void gralloc_handle_set_base(buffer_handle_t handle, uint64_t base)
{
    ((gralloc_handle_t *)handle)->base = base;
}

static inline uint64_t gralloc_handle_phys(buffer_handle_t handle)
{
    return ((gralloc_handle_t *)handle)->phys;
}

static inline void gralloc_handle_set_phys(buffer_handle_t handle, uint64_t phys)
{
    ((gralloc_handle_t *)handle)->phys = phys;
}

static inline uint64_t gralloc_handle_tiling(buffer_handle_t handle)
{
    return ((gralloc_handle_t *)handle)->tiling;
}

static inline uint64_t gralloc_handle_surface(buffer_handle_t handle)
{
    return ((gralloc_handle_t *)handle)->surface;
}

static inline void gralloc_handle_set_surface(buffer_handle_t handle, uint64_t surface)
{
    ((gralloc_handle_t *)handle)->surface = surface;
}

static inline uint32_t gralloc_handle_flags(buffer_handle_t handle)
{
    return ((gralloc_handle_t *)handle)->flags;
}

/* get plane offset in pixels. */
static inline int gralloc_handle_offsets(buffer_handle_t handle, uint32_t plane)
{
    return ((gralloc_handle_t *)handle)->offsets[plane];
}

/* get plane stride in pixels. */
static inline int gralloc_handle_strides(buffer_handle_t handle, uint32_t plane)
{
    return ((gralloc_handle_t *)handle)->strides[plane];
}

/* get plane size in pixels. */
static inline int gralloc_handle_sizes(buffer_handle_t handle, uint32_t plane)
{
    return ((gralloc_handle_t *)handle)->sizes[plane];
}

/* get plane number in pixels. */
static inline int gralloc_handle_num_planes(buffer_handle_t handle)
{
    return ((gralloc_handle_t *)handle)->num_planes;
}

/* get buffer size. */
static inline uint64_t gralloc_handle_size(buffer_handle_t handle)
{
    return ((gralloc_handle_t *)handle)->total_size;
}

#endif
