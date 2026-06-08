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
 * Copyright 2020 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#include "gralloc_utils.h"

#include <array>
#include <unordered_map>

#include <aidl/android/hardware/graphics/common/PlaneLayoutComponent.h>
#include <aidl/android/hardware/graphics/common/PlaneLayoutComponentType.h>
#include <android-base/stringprintf.h>
#include <android-base/strings.h>
#include <cutils/native_handle.h>
#include <gralloctypes/Gralloc4.h>

#include "gralloc_helpers.h"

using aidl::android::hardware::graphics::common::PlaneLayout;
using aidl::android::hardware::graphics::common::PlaneLayoutComponent;
using aidl::android::hardware::graphics::common::PlaneLayoutComponentType;

const std::unordered_map<uint32_t, std::vector<PlaneLayout>>& GetPlaneLayoutsMap() {
    static const auto* kPlaneLayoutsMap =
            new std::unordered_map<uint32_t, std::vector<PlaneLayout>>({
                    {DRM_FORMAT_ABGR8888,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 0,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_G,
                                             .offsetInBits = 8,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_B,
                                             .offsetInBits = 16,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_A,
                                             .offsetInBits = 24,
                                             .sizeInBits = 8}},
                             .sampleIncrementInBits = 32,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_ABGR2101010,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 0,
                                             .sizeInBits = 10},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_G,
                                             .offsetInBits = 10,
                                             .sizeInBits = 10},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_B,
                                             .offsetInBits = 20,
                                             .sizeInBits = 10},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_A,
                                             .offsetInBits = 30,
                                             .sizeInBits = 2}},
                             .sampleIncrementInBits = 32,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_ABGR16161616F,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 0,
                                             .sizeInBits = 16},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_G,
                                             .offsetInBits = 16,
                                             .sizeInBits = 16},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_B,
                                             .offsetInBits = 32,
                                             .sizeInBits = 16},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_A,
                                             .offsetInBits = 48,
                                             .sizeInBits = 16}},
                             .sampleIncrementInBits = 64,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_ARGB8888,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_B,
                                             .offsetInBits = 0,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_G,
                                             .offsetInBits = 8,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 16,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_A,
                                             .offsetInBits = 24,
                                             .sizeInBits = 8}},
                             .sampleIncrementInBits = 32,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_NV12,
                     {{
                              .components = {{.type = android::gralloc4::PlaneLayoutComponentType_Y,
                                              .offsetInBits = 0,
                                              .sizeInBits = 8}},
                              .sampleIncrementInBits = 8,
                              .horizontalSubsampling = 1,
                              .verticalSubsampling = 1,
                      },
                      {
                              .components =
                                      {{.type = android::gralloc4::PlaneLayoutComponentType_CB,
                                        .offsetInBits = 0,
                                        .sizeInBits = 8},
                                       {.type = android::gralloc4::PlaneLayoutComponentType_CR,
                                        .offsetInBits = 8,
                                        .sizeInBits = 8}},
                              .sampleIncrementInBits = 16,
                              .horizontalSubsampling = 2,
                              .verticalSubsampling = 2,
                      }}},

                    {DRM_FORMAT_NV21,
                     {{
                              .components = {{.type = android::gralloc4::PlaneLayoutComponentType_Y,
                                              .offsetInBits = 0,
                                              .sizeInBits = 8}},
                              .sampleIncrementInBits = 8,
                              .horizontalSubsampling = 1,
                              .verticalSubsampling = 1,
                      },
                      {
                              .components =
                                      {{.type = android::gralloc4::PlaneLayoutComponentType_CR,
                                        .offsetInBits = 0,
                                        .sizeInBits = 8},
                                       {.type = android::gralloc4::PlaneLayoutComponentType_CB,
                                        .offsetInBits = 8,
                                        .sizeInBits = 8}},
                              .sampleIncrementInBits = 16,
                              .horizontalSubsampling = 2,
                              .verticalSubsampling = 2,
                      }}},

                    {DRM_FORMAT_P010,
                     {{
                              .components = {{.type = android::gralloc4::PlaneLayoutComponentType_Y,
                                              .offsetInBits = 6,
                                              .sizeInBits = 10}},
                              .sampleIncrementInBits = 16,
                              .horizontalSubsampling = 1,
                              .verticalSubsampling = 1,
                      },
                      {
                              .components =
                                      {{.type = android::gralloc4::PlaneLayoutComponentType_CB,
                                        .offsetInBits = 6,
                                        .sizeInBits = 10},
                                       {.type = android::gralloc4::PlaneLayoutComponentType_CR,
                                        .offsetInBits = 22,
                                        .sizeInBits = 10}},
                              .sampleIncrementInBits = 32,
                              .horizontalSubsampling = 2,
                              .verticalSubsampling = 2,
                      }}},

                    {DRM_FORMAT_P210,
                     {{
                              .components = {{.type = android::gralloc4::PlaneLayoutComponentType_Y,
                                              .offsetInBits = 6,
                                              .sizeInBits = 10}},
                              .sampleIncrementInBits = 16,
                              .horizontalSubsampling = 1,
                              .verticalSubsampling = 1,
                      },
                      {
                              .components =
                                      {{.type = android::gralloc4::PlaneLayoutComponentType_CB,
                                        .offsetInBits = 6,
                                        .sizeInBits = 10},
                                       {.type = android::gralloc4::PlaneLayoutComponentType_CR,
                                        .offsetInBits = 22,
                                        .sizeInBits = 10}},
                              .sampleIncrementInBits = 32,
                              .horizontalSubsampling = 2,
                              .verticalSubsampling = 1,
                      }}},

                    {DRM_FORMAT_R8,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 0,
                                             .sizeInBits = 8}},
                             .sampleIncrementInBits = 8,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_R16,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 0,
                                             .sizeInBits = 16}},
                             .sampleIncrementInBits = 16,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_RGB565,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_B,
                                             .offsetInBits = 0,
                                             .sizeInBits = 5},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_G,
                                             .offsetInBits = 5,
                                             .sizeInBits = 6},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 11,
                                             .sizeInBits = 5}},
                             .sampleIncrementInBits = 16,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_BGR888,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 0,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_G,
                                             .offsetInBits = 8,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_B,
                                             .offsetInBits = 16,
                                             .sizeInBits = 8}},
                             .sampleIncrementInBits = 24,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_XBGR8888,
                     {{
                             .components = {{.type = android::gralloc4::PlaneLayoutComponentType_R,
                                             .offsetInBits = 0,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_G,
                                             .offsetInBits = 8,
                                             .sizeInBits = 8},
                                            {.type = android::gralloc4::PlaneLayoutComponentType_B,
                                             .offsetInBits = 16,
                                             .sizeInBits = 8}},
                             .sampleIncrementInBits = 32,
                             .horizontalSubsampling = 1,
                             .verticalSubsampling = 1,
                     }}},

                    {DRM_FORMAT_YVU420,
                     {
                             {
                                     .components = {{.type = android::gralloc4::
                                                             PlaneLayoutComponentType_Y,
                                                     .offsetInBits = 0,
                                                     .sizeInBits = 8}},
                                     .sampleIncrementInBits = 8,
                                     .horizontalSubsampling = 1,
                                     .verticalSubsampling = 1,
                             },
                             {
                                     .components = {{.type = android::gralloc4::
                                                             PlaneLayoutComponentType_CR,
                                                     .offsetInBits = 0,
                                                     .sizeInBits = 8}},
                                     .sampleIncrementInBits = 8,
                                     .horizontalSubsampling = 2,
                                     .verticalSubsampling = 2,
                             },
                             {
                                     .components = {{.type = android::gralloc4::
                                                             PlaneLayoutComponentType_CB,
                                                     .offsetInBits = 0,
                                                     .sizeInBits = 8}},
                                     .sampleIncrementInBits = 8,
                                     .horizontalSubsampling = 2,
                                     .verticalSubsampling = 2,
                             },
                     }},

                    {DRM_FORMAT_YVU420_ANDROID,
                     {
                             {
                                     .components = {{.type = android::gralloc4::
                                                             PlaneLayoutComponentType_Y,
                                                     .offsetInBits = 0,
                                                     .sizeInBits = 8}},
                                     .sampleIncrementInBits = 8,
                                     .horizontalSubsampling = 1,
                                     .verticalSubsampling = 1,
                             },
                             {
                                     .components = {{.type = android::gralloc4::
                                                             PlaneLayoutComponentType_CR,
                                                     .offsetInBits = 0,
                                                     .sizeInBits = 8}},
                                     .sampleIncrementInBits = 8,
                                     .horizontalSubsampling = 2,
                                     .verticalSubsampling = 2,
                             },
                             {
                                     .components = {{.type = android::gralloc4::
                                                             PlaneLayoutComponentType_CB,
                                                     .offsetInBits = 0,
                                                     .sizeInBits = 8}},
                                     .sampleIncrementInBits = 8,
                                     .horizontalSubsampling = 2,
                                     .verticalSubsampling = 2,
                             },
                     }},
            });
    return *kPlaneLayoutsMap;
}

int getPlaneLayouts(uint32_t drmFormat, std::vector<PlaneLayout>* outPlaneLayouts) {
    const auto& planeLayoutsMap = GetPlaneLayoutsMap();
    const auto it = planeLayoutsMap.find(drmFormat);
    if (it == planeLayoutsMap.end()) {
        ALOGE("Unknown plane layout for format %d", drmFormat);
        return -EINVAL;
    }

    *outPlaneLayouts = it->second;
    return 0;
}
