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

/* Vivante drm based gralloc. */
#ifndef __GRALLOC_VIVANTE_H_
#define __GRALLOC_VIVANTE_H_

#include <hardware/hardware.h>
#include <hardware/gralloc.h>

#include <sys/types.h>
#include <stdlib.h>

#include <stdarg.h>
#include "utils/helpers.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Perform operations. */
#define GRALLOC_VIVANTE_PERFORM_GET_DRM_FD  0x80000002

/* Added temporarily, shall include directly defined enum for mapper hardware interface */
enum Error : int32_t {
    NONE            = 0, /* no error */
    BAD_DESCRIPTOR  = 1, /* invalid BufferDescriptor */
    BAD_BUFFER      = 2, /* invalid buffer handle */
    BAD_VALUE       = 3, /* invalid width, height, etc. */
    /* 4 is reserved */
    NO_RESOURCES    = 5, /* temporary failure due to resource contention */
    /* 6 is reserved */
    UNSUPPORTED     = 7, /* permanent failure */
};

/*
 * Vivante gralloc driver.
 * NOTICE: Do not use below functions out side fo gralloc module.
 */
struct gralloc_vivante_t;

/* Create/destroy vivante gralloc driver. */
int gralloc_vivante_create(gralloc_module_t const *module,
            struct gralloc_vivante_t **pDrv);
void gralloc_vivante_destroy(struct gralloc_vivante_t *drv);

int gralloc_vivante_alloc(struct gralloc_vivante_t *drv, int w, int h,
            int format, uint64_t usage, buffer_handle_t* pHandle, int* pStride);
int gralloc_vivante_free(struct gralloc_vivante_t *drv, buffer_handle_t handle);

int gralloc_vivante_register_buffer(struct gralloc_vivante_t *drv,
            buffer_handle_t handle);
int gralloc_vivante_unregister_buffer(struct gralloc_vivante_t *drv,
            buffer_handle_t handle);
int gralloc_vivante_lock(struct gralloc_vivante_t *drv, buffer_handle_t handle,
            uint64_t usage, int l, int t, int w, int h, void** vaddr);
int gralloc_vivante_unlock(struct gralloc_vivante_t *drv,
            buffer_handle_t handle);
int gralloc_vivante_lock_ycbcr(struct gralloc_vivante_t *drv,
            buffer_handle_t handle, uint64_t usage, int l, int t, int w, int h,
            struct android_ycbcr *ycbcr);
int gralloc_vivante_perform(struct gralloc_vivante_t *drv,
            int operation, va_list args);
int gralloc_vivante_get_transport_size(struct gralloc_vivante_t *drv,
            buffer_handle_t handle, uint32_t *outNumFds, uint32_t *outNumInts);
int gralloc_vivante_validate_buffer_size(buffer_handle_t handle,uint32_t w,
                    uint32_t h,int32_t format,int usage, uint32_t stride);
int gralloc_vivante_set_layerCount(struct gralloc_vivante_t *drv, int layer_count, buffer_handle_t* handle);
#ifdef __cplusplus
}
#endif
#endif
