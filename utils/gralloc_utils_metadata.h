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
 * Copyright 2022 The Chromium OS Authors. All rights reserved.
 * Use of this source code is governed by a BSD-style license that can be
 * found in the LICENSE file.
 */

#ifndef GRALLOC_UTILS_METADATA_H
#define GRALLOC_UTILS_METADATA_H

#include <aidl/android/hardware/graphics/common/StandardMetadataType.h>
#include <aidl/android/hardware/graphics/common/BlendMode.h>
#include <aidl/android/hardware/graphics/common/Cta861_3.h>
#include <aidl/android/hardware/graphics/common/Dataspace.h>
#include <aidl/android/hardware/graphics/common/Smpte2086.h>

#define GRALLOC_METADATA_MAX_NAME_SIZE 1024

struct gralloc_metadata {
    char name[GRALLOC_METADATA_MAX_NAME_SIZE];
    aidl::android::hardware::graphics::common::BlendMode blend_mode;
    aidl::android::hardware::graphics::common::Dataspace dataspace;
    std::optional<aidl::android::hardware::graphics::common::Cta861_3> cta861_3;
    std::optional<aidl::android::hardware::graphics::common::Smpte2086> smpte2086;
};

#endif
