/*
 * Copyright (C) 2019 The Android Open Source Project
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

#pragma once

#include <aidl/android/hardware/graphics/allocator/AllocationResult.h>
#include <aidl/android/hardware/graphics/allocator/BnAllocator.h>
#include <aidl/android/hardware/graphics/allocator/BufferDescriptorInfo.h>
#include <android/hardware/graphics/common/1.2/types.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <log/log.h>

#include <cstdint>
#include <string>
#include <vector>

#include "../../gralloc_manager.h"

using namespace android::hardware::graphics::common::V1_2;

namespace aidl::android::hardware::graphics::allocator::impl {

// from android/hardware/graphics/common/1.2/types.h
BufferUsage enum_values[] {
    //MASK BufferUsage::CPU_READ_MASK,
    BufferUsage::CPU_READ_NEVER,
    BufferUsage::CPU_READ_RARELY,
    BufferUsage::CPU_READ_OFTEN,
    //MASK BufferUsage::CPU_WRITE_MASK,
    BufferUsage::CPU_WRITE_NEVER,
    BufferUsage::CPU_WRITE_RARELY,
    BufferUsage::CPU_WRITE_OFTEN,
    BufferUsage::GPU_TEXTURE,
    BufferUsage::GPU_RENDER_TARGET,
    BufferUsage::COMPOSER_OVERLAY,
    BufferUsage::COMPOSER_CLIENT_TARGET,
    BufferUsage::PROTECTED,
    BufferUsage::COMPOSER_CURSOR,
    BufferUsage::VIDEO_ENCODER,
    BufferUsage::CAMERA_OUTPUT,
    BufferUsage::CAMERA_INPUT,
    BufferUsage::RENDERSCRIPT,
    BufferUsage::VIDEO_DECODER,
    BufferUsage::SENSOR_DIRECT_DATA,
    BufferUsage::GPU_DATA_BUFFER,
    //MASK BufferUsage::VENDOR_MASK,
    //MASK BufferUsage::VENDOR_MASK_HI,
    BufferUsage::GPU_CUBE_MAP,
    BufferUsage::GPU_MIPMAP_COMPLETE,
    BufferUsage::HW_IMAGE_ENCODER,
    (BufferUsage) 4294967296L // Also accepted value: (1 << 32) FRONT_BUFFER from BufferUsage.h AIDL common V4,
};

uint64_t wrong_enum_values[] {
    /**
     * bit 10 must be zero
     */
    1<<10,
    /**
     * bit 13 must be zero
     */
    1<<13,
    /**
     * bit 19 must be zero
     */
    1<<19,
    /**
     * bit 21 must be zero
     */
    1<<21,
    /**
     * bits 33-63 must be zero
     */
};

class VivanteAllocator : public BnAllocator {
  public:
    VivanteAllocator();

    virtual ndk::ScopedAStatus allocate(const std::vector<uint8_t>& descriptor, int32_t count,
                                        allocator::AllocationResult* result) override;
    virtual ndk::ScopedAStatus allocate2(const allocator::BufferDescriptorInfo& descriptor,
                                         int32_t count,
                                         allocator::AllocationResult* result) override;
    virtual ndk::ScopedAStatus isSupported(const allocator::BufferDescriptorInfo& descriptor,
                                           bool* result) override;
    virtual ndk::ScopedAStatus getIMapperLibrarySuffix(std::string* result) override;

  private:
    std::unique_ptr<gralloc_manager> mManager;

    bool allocate_common(const buffer_descriptor_t& bufferDescriptor,
                                       int32_t count, allocator::AllocationResult* outResult);
    bool allocate_internal(buffer_descriptor_t descriptor, int* pStride,
                                         native_handle_t** pHandle);
    void releaseBufferAndHandle(native_handle_t* handle);

};

} // namespace allocator
