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

#ifndef UTIL_H
#define UTIL_H

#define MAX(A, B) ((A) > (B) ? (A) : (B))
#define ARRAY_SIZE(A) (sizeof(A) / sizeof(*(A)))
#define PUBLIC __attribute__((visibility("default")))
#define ALIGN(A, B) (((A) + (B)-1) & ~((B)-1))
#define IS_ALIGNED(A, B) (ALIGN((A), (B)) == (A))
#define DIV_ROUND_UP(n, d) (((n) + (d)-1) / (d))
#define STRINGIZE_NO_EXPANSION(x) #x
#define STRINGIZE(x) STRINGIZE_NO_EXPANSION(x)

#endif
