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

/*
 * Copyright 2016 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef __GRALLOC_HELPERS_H
#define __GRALLOC_HELPERS_H

#include <stdint.h>
#include <string>

#include <system/graphics.h>

#include "drv.h"
#include "helpers.h"
#include "gralloc_vivante_handle.h"
// Reserve the GRALLOC_USAGE_PRIVATE_0 bit from hardware/gralloc.h for buffers
// used for front rendering. minigbm backend later decides to use
// BO_USE_FRONT_RENDERING or BO_USE_LINEAR upon buffer allocaton.
#define BUFFER_USAGE_FRONT_RENDERING (1U << 28)

// Adopt BufferUsage::FRONT_BUFFER from api level 33
#define BUFFER_USAGE_FRONT_RENDERING_MASK (BUFFER_USAGE_FRONT_RENDERING | (1ULL << 32))

int32_t gralloc_sync_wait(int32_t fence, bool close_fence);

uint32_t gralloc_convert_format(uint32_t format);

uint32_t drv_convert_gralloc_format_to_drm_format(int format);

uint64_t gralloc_convert_usage(uint64_t usage);

uint32_t gralloc_convert_map_usage(uint64_t usage);

gralloc_handle_p gralloc_convert_handle(buffer_handle_t handle);

int32_t gralloc_sync_wait(int32_t fence, bool close_fence);

std::string get_drm_format_string(uint32_t drm_format);

uint32_t drv_resolve_format(uint32_t format, uint64_t use_flags);
#endif
