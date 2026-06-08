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
 * Copyright 2014 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef HELPERS_H
#define HELPERS_H

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

#include "drv.h"
#include "gralloc_vivante_handle.h"

#define ALIGN_PIXEL_2(x) ((x + 1) & ~1)
#define ALIGN_PIXEL_4(x) ((x + 3) & ~3)
#define ALIGN_PIXEL_8(x) ((x + 7) & ~7)
#define ALIGN_PIXEL_16(x) ((x + 15) & ~15)
#define ALIGN_PIXEL_32(x) ((x + 31) & ~31)
#define ALIGN_PIXEL_64(x) ((x + 63) & ~63)
#define ALIGN_PIXEL_256(x) ((x + 255) & ~255)

#define BUFFER_NAME_MAX_SIZE 64
#define DRV_MAX_PLANES 4

// temporary add missing format
enum {
    HAL_PIXEL_FORMAT_YCBCR_P210 = 60 /* 0x3C */,
};

enum {
    HAL_PIXEL_FORMAT_R8                 = 0x38,
    HAL_PIXEL_FORMAT_YCbCr_422_P        = 0x100,
    HAL_PIXEL_FORMAT_YCbCr_420_P        = 0x101,
    HAL_PIXEL_FORMAT_CbYCrY_422_I       = 0x102,
    HAL_PIXEL_FORMAT_YCbCr_420_SP       = 0x103,
    HAL_PIXEL_FORMAT_NV12_TILED         = 0x104,
    HAL_PIXEL_FORMAT_NV12_G1_TILED      = 0x105,
    HAL_PIXEL_FORMAT_NV12_G2_TILED      = 0x106,
    HAL_PIXEL_FORMAT_NV12_G2_TILED_COMPRESSED = 0x107,
    HAL_PIXEL_FORMAT_P010                  = 0x108,
    HAL_PIXEL_FORMAT_P010_TILED            = 0x109,
    HAL_PIXEL_FORMAT_P010_TILED_COMPRESSED = 0x110,
};

enum {
    FORMAT_RGBA8888 = 1,
    FORMAT_RGBX8888 = 2,
    FORMAT_RGB888 = 3,
    FORMAT_RGB565 = 4,
    FORMAT_BGRA8888 = 5,
    FORMAT_RGBA1010102 = 0x2B,
    FORMAT_RGBAFP16 = 0x16,
    FORMAT_BLOB = 0x21,
    FORMAT_YCBCR_P010 = 0x36,
    FORMAT_YCBCR_P210 = 0x3C,
    FORMAT_YV12 = 0x32315659, // YCrCb 4:2:0 Planar
    FORMAT_NV16 = 0x10,       // NV16
    FORMAT_NV21 = 0x11,       // NV21
    FORMAT_YUYV = 0x14,       // YUY2
    FORMAT_I420 = 0x101,
    FORMAT_NV12 = 0x103,
    FORMAT_NV12_TILED = 0x104,
    FORMAT_NV12_G1_TILED = 0x105,
    FORMAT_NV12_G2_TILED = 0x106,
    FORMAT_NV12_G2_TILED_COMPRESSED = 0x107,
    FORMAT_P010 = 0x108,
    FORMAT_P010_TILED = 0x109,
    FORMAT_P010_TILED_COMPRESSED = 0x110,
    FORMAT_RAW16 = 0x203,
};

int convert_pix_format_to_drm_format(int format);
uint32_t drv_height_from_format(uint32_t format, uint32_t height, size_t plane);
uint32_t drv_vertical_subsampling_from_format(uint32_t format, size_t plane);
uint32_t drv_size_from_format(uint32_t format, uint32_t stride, uint32_t height, size_t plane);
int drv_bo_from_format(gralloc_handle_p hnd, uint32_t stride, uint32_t aligned_height,
                       uint32_t format);
int drv_bo_from_format_and_padding(gralloc_handle_p hnd, uint32_t stride, uint32_t aligned_height,
                                   uint32_t format, uint32_t padding[DRV_MAX_PLANES]);
#endif
