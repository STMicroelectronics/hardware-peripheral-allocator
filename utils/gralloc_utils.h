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

#include <string>
#include <vector>

#include <aidl/android/hardware/graphics/common/PlaneLayout.h>
#include <android/hardware/graphics/common/1.2/types.h>

int getPlaneLayouts(
        uint32_t drm_format,
        std::vector<aidl::android::hardware::graphics::common::PlaneLayout>* out_layouts);
