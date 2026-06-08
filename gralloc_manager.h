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

#ifndef __GRALLOC_MANAGER_H
#define __GRALLOC_MANAGER_H

#include <functional>
#include <mutex>
#include <unordered_map>

#include <hardware/gralloc.h>
#include "utils/gralloc_helpers.h"
#include "gralloc_vivante.h"
#include "gralloc_vivante_handle.h"

#include <aidl/android/hardware/graphics/common/Dataspace.h>
using aidl::android::hardware::graphics::common::Dataspace;

/* A internal buffer_descriptor contains the requested parameters for the buffer
 * as well as the calculated parameters that are passed to the Vivante allocator.
 */
struct buffer_descriptor_t
{
    uint32_t width;
    uint32_t height;
    uint32_t hal_format;
    uint32_t layer_count;
    uint64_t buffer_usage;
    uint64_t use_flags;
    uint64_t reserved_region_size;
    std::string name;
    Dataspace dataspace = Dataspace::UNKNOWN;
};

class gralloc_manager
{
public:

    gralloc_manager();
    ~gralloc_manager();

    int32_t init();
    int32_t allocate(const buffer_descriptor_t* descriptor,
                     buffer_handle_t *out_handle, int* pStride);
    int32_t free(buffer_handle_t handle);
    int32_t release(buffer_handle_t handle);

    int32_t get_reserved_region(buffer_handle_t handle,
                                void **reserved_region_addr, uint64_t *reserved_region_size);

    gralloc_module_t *mGPUModule;

    std::unordered_map<gralloc_handle_p, void *> reserved_region_addrs;

private:

    int create_reserved_region(const std::string &buffer_name, uint64_t reserved_region_size);
    int32_t release_region(buffer_handle_t handle);
    void list_region_elements(void);

    pthread_mutex_t reserved_region_addrs_lock = PTHREAD_MUTEX_INITIALIZER;
    alloc_device_t   *mGPUAlloc;
    std::mutex mutex_;

};

#endif
