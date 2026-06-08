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

#undef LOG_TAG
#define LOG_TAG "gralloc-stm-allocator"

#include "allocator.h"

#include <aidl/android/hardware/graphics/allocator/AllocationError.h>
#include <android/hardware/graphics/mapper/IMapper.h>
#include <aidlcommonsupport/NativeHandle.h>
#include <android/binder_ibinder.h>
#include <android/binder_status.h>
#include <cutils/android_filesystem_config.h>
#include <gralloctypes/Gralloc4.h>
#include <hardware/gralloc.h>
#include <hardware/hardware.h>
#include <sys/mman.h>

#include "utils/gralloc_helpers.h"
#include "../../gralloc_manager.h"
#include "utils/gralloc_utils_metadata.h"
#include "gralloc_vivante.h"
#include "gralloc_vivante_handle.h"

using BufferDescriptorInfoV4 =
        android::hardware::graphics::mapper::V4_0::IMapper::BufferDescriptorInfo;
using android::hardware::graphics::common::V1_2::BufferUsage;
using android::hardware::graphics::common::V1_2::PixelFormat;
using aidl::android::hardware::graphics::common::BlendMode;
using aidl::android::hardware::graphics::common::Dataspace;
using ndk::internal::enum_values;

static const std::string STANDARD_METADATA_DATASPACE = "android.hardware.graphics.common.Dataspace";

namespace aidl::android::hardware::graphics::allocator::impl {
namespace {
inline ndk::ScopedAStatus ToBinderStatus(AllocationError error) {
    return ndk::ScopedAStatus::fromServiceSpecificError(static_cast<int32_t>(error));
}
constexpr bool usageTest(const BufferUsage a, const BufferUsage b) {
    //ALOGV("test %lu vs %lu", static_cast<uint64_t>(a), static_cast<uint64_t>(b));
    return (static_cast<uint64_t>(a) & static_cast<uint64_t>(b)) != 0;
}
bool usageTestLoop(const BufferUsage a) {
    bool usageOk = false;
    if (a == BufferUsage::CPU_READ_NEVER || a == BufferUsage::CPU_WRITE_NEVER) {
        ALOGV("Found buffer usage for value %lu", (uint64_t)a);
        usageOk = true;
    }
    /* found at least one good non zero value */
    for (const auto& value : enum_values) {
        if (usageTest(a, (const BufferUsage) value)) {
            ALOGV("Found buffer usage for value %lu", value);
            usageOk = true;
            break;
        }
    }
    if (usageOk == true) {
        /* did not find any wrong value */
        for (const auto& value : wrong_enum_values) {
            if (usageTest(a, (const BufferUsage) value)) {
                ALOGE("Found wrong buffer usage for value %lu", value);
                usageOk = false;
                break;
            }
        }
        /* did not find any bit above 32 */
        if (usageTest(a, (const BufferUsage) 0xFFFFFFFE00000000)) {
            ALOGE("Found wrong buffer usage for value %lu (bit set over position 32)", a);
            usageOk = false;
        }
        /* FRONT_BUFFER not supported by Vivante */
        if (usageTest(a, (const BufferUsage) 4294967296L)) {
            ALOGE("FRONT_BUFFER buffer not supported");
            usageOk = false;
        }
    }

    return usageOk;
}
bool toInternalDescriptor(const BufferDescriptorInfoV4& info,
                          struct buffer_descriptor_t* bufferDescriptor) {

    /* check w/h and layerCount info */
    if (info.width == 0 || info.height == 0 || info.layerCount == 0)
    {
        ALOGE("buffer descriptor not supported with width = %d, height = %d, layerCount = %d",
              info.width, info.height, info.layerCount);
        return true;
    }

    /* Check layerCount max value */
    if (info.layerCount != 1)
    {
        ALOGE("layerCount = %d > 1 is not supported", info.layerCount);
        return true;
    }

    /* check format */
    int drmFormat = gralloc_convert_format(static_cast<uint32_t>(info.format));
    if(drmFormat == DRM_FORMAT_NONE) {
        ALOGE("HAL format (%d) not supported", info.format);
        return true;
    }

    /* check usage */
    if (!usageTestLoop((const BufferUsage) info.usage))
    {
        ALOGE("buffer descriptor usage (%lu) not supported", static_cast<uint64_t>(info.usage));
        return true;
    }
    else {
        ALOGV("buffer descriptor usage (%lu) is supported", static_cast<uint64_t>(info.usage));
    }

    bufferDescriptor->name = info.name;
    bufferDescriptor->width = info.width;
    bufferDescriptor->height = info.height;
    bufferDescriptor->hal_format = static_cast<uint32_t>(info.format);
    bufferDescriptor->buffer_usage = info.usage;
    bufferDescriptor->reserved_region_size = info.reservedSize + sizeof(gralloc_metadata);
    ALOGV("reserved_region_size=%lu = reservedSize(%lu) + gralloc_metadata(%lu)",
          bufferDescriptor->reserved_region_size, info.reservedSize, sizeof(gralloc_metadata));

    return false;
}

} // namespace

VivanteAllocator::VivanteAllocator() : mManager(std::make_unique<gralloc_manager>()) {
    if (!mManager->init()) {
        ALOGE("Failed to initialize gralloc manager.");
        mManager = nullptr;
    }
}

void VivanteAllocator::releaseBufferAndHandle(native_handle_t* handle) {
    int ret = mManager->free(handle);
    if (ret != 0) {
        ALOGE("release handle failed");
    }
    //native_handle_close(handle);
    //native_handle_delete(handle);
}

bool VivanteAllocator::allocate_internal(buffer_descriptor_t bufferDescriptor, int* pStride,
                                                       native_handle_t** pHandle) {

    int ret = mManager->allocate(&bufferDescriptor, (buffer_handle_t *)pHandle, pStride);

    if (ret) {
        ALOGE("Failed in Vivante device allocation (w=%d, h=%d, format=%lu, usage=%ld)\n",
              bufferDescriptor.width, bufferDescriptor.height,
              static_cast<uint64_t>(bufferDescriptor.hal_format),
              static_cast<uint64_t>(bufferDescriptor.buffer_usage));
        *pHandle = nullptr;
        return false;
    }
    else {
        gralloc_handle_p hnd = (gralloc_handle_p) *pHandle;

        ALOGV("Vivante device allocation (w=%d, h=%d, format=%lu, usage=%ld, fd_region=%d)\n",
              bufferDescriptor.width, bufferDescriptor.height,
              static_cast<uint64_t>(bufferDescriptor.hal_format),
              static_cast<uint64_t>(bufferDescriptor.buffer_usage),
              hnd->fd_region);
        return true;
    }
}

bool VivanteAllocator::allocate_common(
        const buffer_descriptor_t& bufferDescriptor, int32_t count,
        allocator::AllocationResult* outResult) {

    std::vector<native_handle_t*> handles;
    handles.resize(count, nullptr);

    for (int32_t i = 0; i < count; i++) {
        bool status = allocate_internal(bufferDescriptor, &outResult->stride, &handles[i]);
        if (!status) {
            for (int32_t j = 0; j < i; j++) {
                releaseBufferAndHandle(handles[j]);
            }
            return status;
        }
    }

    outResult->buffers.resize(count);
    for (int32_t i = 0; i < count; i++) {
        auto handle = handles[i];
        outResult->buffers[i] = ::android::dupToAidl(handle);
        releaseBufferAndHandle(handle);
    }

    return true;

}

ndk::ScopedAStatus VivanteAllocator::allocate(const std::vector<uint8_t>& descriptor, int32_t count,
                                              allocator::AllocationResult* result) {

    ALOGV("allocate in");

    if (!mManager) {
        ALOGE("Failed to allocate. Gralloc manager is uninitialized.");
        return ToBinderStatus(AllocationError::NO_RESOURCES);
    }

    BufferDescriptorInfoV4 mapperV4Descriptor;

    int ret = ::android::gralloc4::decodeBufferDescriptorInfo(descriptor, &mapperV4Descriptor);
    if (ret) {
        ALOGE("Failed to allocate. Failed to decode buffer descriptor: %d.\n", ret);
        return ToBinderStatus(AllocationError::BAD_DESCRIPTOR);
    }

    buffer_descriptor_t bufferDescriptor = {};
    if (toInternalDescriptor(mapperV4Descriptor, &bufferDescriptor)) {
        ALOGE("toInternalDescriptor error");
        return ToBinderStatus(AllocationError::UNSUPPORTED);
    }

    if(allocate_common(bufferDescriptor, count, result)) {
        ALOGV("Allocate done width=%d, height=%d, layerCount=%d, format=%d, usage=%lu, reservedSized=%lu",
              mapperV4Descriptor.width, mapperV4Descriptor.height,
              mapperV4Descriptor.layerCount, mapperV4Descriptor.format,
              mapperV4Descriptor.usage, mapperV4Descriptor.reservedSize);
        return ndk::ScopedAStatus::ok();
    }
    else {
        return ToBinderStatus(AllocationError::NO_RESOURCES);
    }
}

ndk::ScopedAStatus VivanteAllocator::allocate2(
        const allocator::BufferDescriptorInfo& descriptor, int32_t count,
        allocator::AllocationResult* result) {

    ALOGV("allocate2 in");

    if (!mManager) {
        ALOGE("Failed to allocate2. Gralloc manager is uninitialized.");
        return ToBinderStatus(AllocationError::NO_RESOURCES);
    }

    const BufferDescriptorInfoV4 mapperV4Descriptor = {
            .name{reinterpret_cast<const char*>(descriptor.name.data())},
            .width = static_cast<uint32_t>(descriptor.width),
            .height = static_cast<uint32_t>(descriptor.height),
            .layerCount = static_cast<uint32_t>(descriptor.layerCount),
            .format = static_cast<::android::hardware::graphics::common::V1_2::PixelFormat>(descriptor.format),
            .usage = static_cast<uint64_t>(descriptor.usage),
            .reservedSize = static_cast<uint64_t>(descriptor.reservedSize),
    };

    buffer_descriptor_t bufferDescriptor = {};
    if (toInternalDescriptor(mapperV4Descriptor, &bufferDescriptor)) {
        ALOGE("toInternalDescriptor error");
        return ToBinderStatus(AllocationError::UNSUPPORTED);
    }

    /* check additionalOptions */
    for (const auto& option : descriptor.additionalOptions) {
        if (option.name != STANDARD_METADATA_DATASPACE) {
            ALOGE("Option name is not Dataspace");
            return ToBinderStatus(AllocationError::UNSUPPORTED);
        }
    }

    if(allocate_common(bufferDescriptor, count, result)) {
        ALOGI("Allocate2 done width=%d, height=%d, layerCount=%d, format=%d, usage=%lu, reservedSized=%lu",
              mapperV4Descriptor.width, mapperV4Descriptor.height,
              mapperV4Descriptor.layerCount, mapperV4Descriptor.format,
              mapperV4Descriptor.usage, mapperV4Descriptor.reservedSize);
        return ndk::ScopedAStatus::ok();
    }
    else {
        ALOGE("allocate_common error");
        return ToBinderStatus(AllocationError::UNSUPPORTED);
    }
}

ndk::ScopedAStatus VivanteAllocator::isSupported(
        const allocator::BufferDescriptorInfo& descriptor, bool* result) {

    ALOGV("isSupported in");

    *result=true;

    const BufferDescriptorInfoV4 mapperV4Descriptor = {
            .name{reinterpret_cast<const char*>(descriptor.name.data())},
            .width = static_cast<uint32_t>(descriptor.width),
            .height = static_cast<uint32_t>(descriptor.height),
            .layerCount = static_cast<uint32_t>(descriptor.layerCount),
            .format = static_cast<::android::hardware::graphics::common::V1_2::PixelFormat>(descriptor.format),
            .usage = static_cast<uint64_t>(descriptor.usage),
            .reservedSize = static_cast<uint64_t>(descriptor.reservedSize),
    };

    buffer_descriptor_t bufferDescriptor = {};
    if (toInternalDescriptor(mapperV4Descriptor, &bufferDescriptor)) {
        *result=false;
    }

    /* check additionalOptions */
    for (const auto& option : descriptor.additionalOptions) {
        if (option.name != STANDARD_METADATA_DATASPACE) {
            ALOGE("Option name is not Dataspace");
            *result=false;
        }
    }

    return ndk::ScopedAStatus::ok();
}

ndk::ScopedAStatus VivanteAllocator::getIMapperLibrarySuffix(std::string* result) {
    *result = "stm";
    return ndk::ScopedAStatus::ok();
}

} // namespace aidl::android::hardware::graphics::allocator::impl

