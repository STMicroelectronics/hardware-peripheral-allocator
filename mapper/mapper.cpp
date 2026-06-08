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

#include <aidl/android/hardware/graphics/allocator/BufferDescriptorInfo.h>
#include <aidl/android/hardware/graphics/common/BufferUsage.h>
#include <aidl/android/hardware/graphics/common/PixelFormat.h>
#include <aidl/android/hardware/graphics/common/StandardMetadataType.h>
#include <android-base/unique_fd.h>
#include <android/hardware/graphics/mapper/IMapper.h>
#include <android/hardware/graphics/mapper/utils/IMapperMetadataTypes.h>
#include <android/hardware/graphics/mapper/utils/IMapperProvider.h>
#include <android/hardware/graphics/common/1.2/types.h>
#include <cutils/native_handle.h>
#include <gralloctypes/Gralloc4.h>
#include <log/log.h>

#include <memory>

#include <hardware/gralloc.h>
#include <sync/sync.h>
#include <ui/Fence.h>

#include "gralloc_vivante_handle.h"
#include "gralloc_vivante.h"
#include "utils/gralloc_utils.h"
#include "utils/gralloc_helpers.h"
#include "../gralloc_manager.h"
#include "utils/gralloc_utils_metadata.h"

#undef LOG_TAG
#define LOG_TAG "gralloc-stm-mapper"

#define GRALLOC_BUFFER_METADATA_MAX_NAME_SIZE 1024

#ifndef UNUSED
#define UNUSED(x) (void)(x)
#endif

using namespace ::aidl::android::hardware::graphics::common;
using namespace ::android::hardware::graphics::mapper;
using ::aidl::android::hardware::graphics::allocator::BufferDescriptorInfo;
using ::android::base::unique_fd;

#define VALIDATE_BUFFER_HANDLE(bufferHandle)                    \
    if (!(bufferHandle)) {                                      \
        ALOGE("Failed to %s. Null buffer_handle_t.", __func__); \
        return AIMAPPER_ERROR_BAD_BUFFER;                       \
    }

static_assert(GRALLOC_BUFFER_METADATA_MAX_NAME_SIZE >=
                      decltype(std::declval<BufferDescriptorInfo>().name){}.size(),
              "Metadata name storage too small to fit a BufferDescriptorInfo::name");

constexpr const char* STANDARD_METADATA_NAME =
        "android.hardware.graphics.common.StandardMetadataType";

static bool isStandardMetadata(AIMapper_MetadataType metadataType) {
    return strcmp(STANDARD_METADATA_NAME, metadataType.name) == 0;
}

class VivanteMapper final : public vendor::mapper::IMapperV5Impl {

  public:
    VivanteMapper();
    ~VivanteMapper();

    AIMapper_Error importBuffer(const native_handle_t* _Nonnull handle,
                                buffer_handle_t _Nullable* _Nonnull outBufferHandle) override;

    AIMapper_Error freeBuffer(buffer_handle_t _Nonnull buffer) override;

    AIMapper_Error getTransportSize(buffer_handle_t _Nonnull buffer, uint32_t* _Nonnull outNumFds,
                                    uint32_t* _Nonnull outNumInts) override;

    AIMapper_Error lock(buffer_handle_t _Nonnull buffer, uint64_t cpuUsage, ARect accessRegion,
                        int acquireFence, void* _Nullable* _Nonnull outData) override;

    AIMapper_Error unlock(buffer_handle_t _Nonnull buffer, int* _Nonnull releaseFence) override;

    AIMapper_Error flushLockedBuffer(buffer_handle_t _Nonnull buffer) override;

    AIMapper_Error rereadLockedBuffer(buffer_handle_t _Nonnull buffer) override;

    int32_t getMetadata(buffer_handle_t _Nonnull buffer, AIMapper_MetadataType metadataType,
                        void* _Nonnull outData, size_t outDataSize) override;

    int32_t getStandardMetadata(buffer_handle_t _Nonnull buffer, int64_t standardMetadataType,
                                void* _Nonnull outData, size_t outDataSize) override;

    AIMapper_Error setMetadata(buffer_handle_t _Nonnull buffer, AIMapper_MetadataType metadataType,
                               const void* _Nonnull metadata, size_t metadataSize) override;

    AIMapper_Error setStandardMetadata(buffer_handle_t _Nonnull bufferHandle,
                                       int64_t standardMetadataType, const void* _Nonnull metadata,
                                       size_t metadataSize) override;

    AIMapper_Error listSupportedMetadataTypes(
            const AIMapper_MetadataTypeDescription* _Nullable* _Nonnull outDescriptionList,
            size_t* _Nonnull outNumberOfDescriptions) override;

    AIMapper_Error dumpBuffer(buffer_handle_t _Nonnull bufferHandle,
                              AIMapper_DumpBufferCallback _Nonnull dumpBufferCallback,
                              void* _Null_unspecified context) override;

    AIMapper_Error dumpAllBuffers(AIMapper_BeginDumpBufferCallback _Nonnull beginDumpBufferCallback,
                                  AIMapper_DumpBufferCallback _Nonnull dumpBufferCallback,
                                  void* _Null_unspecified context) override;

    AIMapper_Error getReservedRegion(buffer_handle_t _Nonnull buffer,
                                     void* _Nullable* _Nonnull outReservedRegion,
                                     uint64_t* _Nonnull outReservedSize) override;

  private:
    std::unique_ptr<gralloc_manager> mManager;

    template <typename F, StandardMetadataType TYPE>
    int32_t getStandardMetadata(buffer_handle_t _Nonnull bufferHandle, F&& provide,
                                StandardMetadata<TYPE>);

    template <StandardMetadataType TYPE>
    AIMapper_Error setStandardMetadata(buffer_handle_t _Nonnull bufferHandle,
                                       typename StandardMetadata<TYPE>::value_type&& value);

    int32_t get_metadata(buffer_handle_t bufferHandle, struct gralloc_metadata **metadata);
    int32_t get_metadata(buffer_handle_t bufferHandle, const struct gralloc_metadata **metadata);

    int32_t get_name(buffer_handle_t bufferHandle, std::optional<std::string> *name);

    int32_t get_blend_mode(buffer_handle_t bufferHandle,
                           std::optional<aidl::android::hardware::graphics::common::BlendMode> *blend_mode);
    int32_t set_blend_mode(buffer_handle_t bufferHandle,
                           aidl::android::hardware::graphics::common::BlendMode blend_mode);

    int32_t get_dataspace(buffer_handle_t bufferHandle,
                          std::optional<aidl::android::hardware::graphics::common::Dataspace> *dataspace);
    int32_t set_dataspace(buffer_handle_t bufferHandle,
                          aidl::android::hardware::graphics::common::Dataspace dataspace);

    int32_t get_cta861_3(buffer_handle_t bufferHandle,
                         std::optional<aidl::android::hardware::graphics::common::Cta861_3> *cta);
    int32_t set_cta861_3(buffer_handle_t bufferHandle,
                         std::optional<aidl::android::hardware::graphics::common::Cta861_3> cta);

    int32_t get_smpte2086(buffer_handle_t bufferHandle,
                          std::optional<aidl::android::hardware::graphics::common::Smpte2086> *smpte);
    int32_t set_smpte2086(buffer_handle_t bufferHandle,
                          std::optional<aidl::android::hardware::graphics::common::Smpte2086> smpte);

};

VivanteMapper::VivanteMapper() : mManager(std::make_unique<gralloc_manager>()) {
    if (!mManager->init()) {
        ALOGE("Failed to initialize gralloc manager.");
        mManager = nullptr;
    }
}

VivanteMapper::~VivanteMapper() {
}

AIMapper_Error VivanteMapper::importBuffer(
        const native_handle_t* _Nonnull bufferHandle,
        buffer_handle_t _Nullable* _Nonnull outBufferHandle) {

    ALOGV("importBuffer in");

    if (!mManager) {
        ALOGE("Failed to importBuffer. Gralloc manager is uninitialized.");
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    if (!bufferHandle || bufferHandle->numFds == 0) {
        ALOGE("Failed to importBuffer. Bad handle.");
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    native_handle_t* importedBufferHandle = native_handle_clone(bufferHandle);
    if (!importedBufferHandle) {
        ALOGE("Failed to importBuffer. Handle clone failed: %s.", strerror(errno));
        return AIMAPPER_ERROR_NO_RESOURCES;
    }

    int ret = mManager->mGPUModule->registerBuffer(mManager->mGPUModule, importedBufferHandle);
    if (ret) {
        native_handle_close(importedBufferHandle);
        native_handle_delete(importedBufferHandle);
        ALOGE("Failed to registerBuffer");
        return AIMAPPER_ERROR_NO_RESOURCES;
    }

    *outBufferHandle = importedBufferHandle;
    return AIMAPPER_ERROR_NONE;
}

AIMapper_Error VivanteMapper::freeBuffer(buffer_handle_t _Nonnull buffer) {
    ALOGV("freeBuffer in");
    VALIDATE_BUFFER_HANDLE(buffer)

    if (!mManager) {
        ALOGE("Failed to freeBuffer. Gralloc manager is uninitialized.");
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    int ret = mManager->mGPUModule->unregisterBuffer(mManager->mGPUModule, buffer);
    if (ret) {
        ALOGE("Failed to unregisterBuffer");
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    ret = mManager->release(buffer);
    if (ret != 0) {
        ALOGE("release memory failed");
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    native_handle_close(buffer);
    native_handle_delete(const_cast<native_handle_t*>(buffer));
    return AIMAPPER_ERROR_NONE;
}

AIMapper_Error VivanteMapper::getTransportSize(buffer_handle_t _Nonnull bufferHandle,
                                                     uint32_t* _Nonnull outNumFds,
                                                     uint32_t* _Nonnull outNumInts) {
    ALOGV("getTransportSize in");
    VALIDATE_BUFFER_HANDLE(bufferHandle)

    if (!mManager) {
        ALOGE("Failed to getTransportSize. Gralloc manager is uninitialized.");
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    int ret = mManager->mGPUModule->getTransportSize(mManager->mGPUModule, bufferHandle, outNumFds, outNumInts);
    if (ret) {
        ALOGE("Failed to getTransportSize");
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    ALOGV("getTransportSize done outNumFds=%d, outNumInts=%d", *outNumFds, *outNumInts);
    return AIMAPPER_ERROR_NONE;
}

static void waitFenceFd(const android::base::unique_fd& fenceFd, const char* logname) {
    if (fenceFd < 0) {
        return;
    }

    const int warningTimeout = 3500;
    const int error = sync_wait(fenceFd, warningTimeout);
    if (error < 0 && errno == ETIME) {
        ALOGE("%s: fence %d didn't signal in %u ms", logname, fenceFd.get(), warningTimeout);
        sync_wait(fenceFd, -1);
    }
}

AIMapper_Error VivanteMapper::lock(buffer_handle_t _Nonnull bufferHandle, uint64_t cpuUsage,
                                         ARect region, int acquireFenceRawFd,
                                         void* _Nullable* _Nonnull outData) {
    ALOGV("lock in");
    unique_fd acquireFence(acquireFenceRawFd);
    VALIDATE_BUFFER_HANDLE(bufferHandle)

    if (!mManager) {
        ALOGE("Failed to lock. Gralloc manager is uninitialized.");
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    if (cpuUsage == 0) {
        ALOGE("Failed to lock. Bad cpu usage: %" PRIu64 ".", cpuUsage);
        return AIMAPPER_ERROR_BAD_VALUE;
    }

    struct rectangle rect;

    // An access region of all zeros means the entire buffer.
    if (region.left == 0 && region.top == 0 && region.right == 0 && region.bottom == 0) {
        rect = {0, 0, (uint32_t)gralloc_handle_width(bufferHandle), (uint32_t)gralloc_handle_height(bufferHandle)};
    } else {
        if (region.left < 0 || region.top < 0 || region.right <= region.left ||
            region.bottom <= region.top) {
            ALOGE("Failed to lock. Invalid accessRegion: [%d, %d, %d, %d]", region.left, region.top,
            region.right, region.bottom);
            return AIMAPPER_ERROR_BAD_VALUE;
    }

    if (region.right > gralloc_handle_width(bufferHandle)) {
        ALOGE("Failed to lock. Invalid region: width greater than buffer width (%d vs %d).",
        region.right, gralloc_handle_width(bufferHandle));
        return AIMAPPER_ERROR_BAD_VALUE;
    }

    if (region.bottom > gralloc_handle_height(bufferHandle)) {
        ALOGE("Failed to lock. Invalid region: height greater than buffer height (%d vs " "%d).",
                region.bottom, gralloc_handle_height(bufferHandle));
        return AIMAPPER_ERROR_BAD_VALUE;
    }

    rect = {static_cast<uint32_t>(region.left), static_cast<uint32_t>(region.top),
            static_cast<uint32_t>(region.right - region.left),
            static_cast<uint32_t>(region.bottom - region.top)};
    }

    uint8_t* addr[DRV_MAX_PLANES];
    waitFenceFd(acquireFence, "GrallocMapper::lock");
    int32_t status = mManager->mGPUModule->lock(mManager->mGPUModule, bufferHandle, cpuUsage, region.left, region.top,
                            gralloc_handle_width(bufferHandle), gralloc_handle_height(bufferHandle),
                            (void **)addr);
    if (status) {
        ALOGE("Failed to lock");
        return AIMAPPER_ERROR_BAD_VALUE;
    }

    *outData = addr[0];
    return AIMAPPER_ERROR_NONE;
}

AIMapper_Error VivanteMapper::unlock(buffer_handle_t _Nonnull buffer,
                                           int* _Nonnull releaseFence) {
    ALOGV("unlock in");
    VALIDATE_BUFFER_HANDLE(buffer)

    if (!mManager) {
        ALOGE("Failed to unlock. Gralloc manager is uninitialized.");
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    int ret = mManager->mGPUModule->unlock(mManager->mGPUModule, buffer);
    *releaseFence = -1;
    if (ret) {
        ALOGE("Failed to unlock");
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    return AIMAPPER_ERROR_NONE;
}

AIMapper_Error VivanteMapper::flushLockedBuffer(buffer_handle_t _Nonnull buffer __unused) {
    ALOGV("flushLockedBuffer in");
    VALIDATE_BUFFER_HANDLE(buffer)

    if (!mManager) {
        ALOGE("Failed to flushLockedBuffer. Gralloc manager is uninitialized.");
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    /*Todo: Add gralloc flush operation. */
    //int ret = mManager->mGPUModule->flush(mManager->mGPUModule, buffer);

    return AIMAPPER_ERROR_NONE;
}

AIMapper_Error VivanteMapper::rereadLockedBuffer(buffer_handle_t _Nonnull buffer __unused) {
    ALOGV("rereadLockedBuffer in");
    VALIDATE_BUFFER_HANDLE(buffer)

    if (!mManager) {
        ALOGE("Failed to rereadLockedBuffer. Gralloc manager is uninitialized.");
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    /*Todo: Add gralloc invalidate operation. */
    //int ret = mManager->mGPUModule->invalidate(mManager->mGPUModule, buffer);

    return AIMAPPER_ERROR_NONE;
}

int32_t VivanteMapper::getMetadata(buffer_handle_t _Nonnull bufferHandle,
                                         AIMapper_MetadataType metadataType, void* _Nonnull outData,
                                         size_t outDataSize) {
    ALOGV("getMetadata in");
    if (!(bufferHandle)) {                                      \
        ALOGE("Failed to %s. Null buffer_handle_t.", __func__); \
        return -AIMAPPER_ERROR_BAD_BUFFER;                       \
    }

    if (!mManager) {
        ALOGE("Failed to getMetadata. Gralloc manager is uninitialized.");
        return -AIMAPPER_ERROR_BAD_BUFFER;
    }
    // We don't have any vendor-specific metadata, so divert to getStandardMetadata after validating
    // that this is a standard metadata request
    if (isStandardMetadata(metadataType)) {
        return getStandardMetadata(bufferHandle, metadataType.value, outData, outDataSize);
    }
    return -AIMAPPER_ERROR_UNSUPPORTED;
}

int32_t VivanteMapper::getStandardMetadata(buffer_handle_t _Nonnull bufferHandle,
                                           int64_t standardType, void* _Nonnull outData,
                                           size_t outDataSize) {
    ALOGV("getStandardMetadata in");
    if (!(bufferHandle)) {                                      \
        ALOGE("Failed to %s. Null buffer_handle_t.", __func__); \
        return -AIMAPPER_ERROR_BAD_BUFFER;                       \
    }

    int32_t retValue = -AIMAPPER_ERROR_UNSUPPORTED;
    auto provider = [&]<StandardMetadataType T>(auto&& provide) -> int32_t {
        return getStandardMetadata(bufferHandle, provide, StandardMetadata<T>{});
    };
    retValue = provideStandardMetadata(static_cast<StandardMetadataType>(standardType), outData,
                                       outDataSize, provider);
    return retValue;
}

template <typename F, StandardMetadataType metadataType>
int32_t VivanteMapper::getStandardMetadata(buffer_handle_t _Nonnull bufferHandle, F&& provide,
                                           StandardMetadata<metadataType>) {
    if constexpr (metadataType == StandardMetadataType::BUFFER_ID) {
        return provide(gralloc_handle_id(bufferHandle));
    }
    if constexpr (metadataType == StandardMetadataType::NAME) {
        char name[BUFFER_NAME_MAX_SIZE];
        if (gralloc_handle_name(bufferHandle, name)) {
            return -AIMAPPER_ERROR_NO_RESOURCES;
        } else {
            return provide(name);
        }
    }
    if constexpr (metadataType == StandardMetadataType::WIDTH) {
        return provide(gralloc_handle_width(bufferHandle));
    }
    if constexpr (metadataType == StandardMetadataType::STRIDE) {
        return provide(gralloc_handle_stride(bufferHandle));
    }
    if constexpr (metadataType == StandardMetadataType::HEIGHT) {
        return provide(gralloc_handle_height(bufferHandle));
    }
    if constexpr (metadataType == StandardMetadataType::LAYER_COUNT) {
        return provide(1);
    }
    if constexpr (metadataType == StandardMetadataType::PIXEL_FORMAT_REQUESTED) {
        return provide(static_cast<PixelFormat>(gralloc_handle_format(bufferHandle)));
    }
    if constexpr (metadataType == StandardMetadataType::PIXEL_FORMAT_FOURCC) {
        return provide(gralloc_convert_format(gralloc_handle_format(bufferHandle)));
    }
    if constexpr (metadataType == StandardMetadataType::PIXEL_FORMAT_MODIFIER) {
        return provide(gralloc_handle_format_modifier(bufferHandle));
    }
    if constexpr (metadataType == StandardMetadataType::USAGE) {
        return provide(static_cast<BufferUsage>(gralloc_handle_usage(bufferHandle)));
    }
    if constexpr (metadataType == StandardMetadataType::ALLOCATION_SIZE) {
        return provide(gralloc_handle_size(bufferHandle));
    }
    if constexpr (metadataType == StandardMetadataType::PROTECTED_CONTENT) {
        uint64_t hasProtectedContent = (static_cast<uint64_t>(gralloc_handle_usage(bufferHandle)))
                                      & static_cast<uint64_t>(BufferUsage::PROTECTED) ? 1 : 0;
        return provide(hasProtectedContent);
    }
    if constexpr (metadataType == StandardMetadataType::COMPRESSION) {
        return provide(android::gralloc4::Compression_None);
    }
    if constexpr (metadataType == StandardMetadataType::INTERLACED) {
        return provide(android::gralloc4::Interlaced_None);
    }
    if constexpr (metadataType == StandardMetadataType::CHROMA_SITING) {
        return provide(android::gralloc4::ChromaSiting_None);
    }
    if constexpr (metadataType == StandardMetadataType::PLANE_LAYOUTS) {
        std::vector<PlaneLayout> planeLayouts;
        uint32_t drm_format = gralloc_convert_format(gralloc_handle_format(bufferHandle));
        if (drm_format == DRM_FORMAT_NONE) {
            ALOGE("Failed to resolve DRM format for gralloc format 0x%x",
                  gralloc_handle_format(bufferHandle));
            return -AIMAPPER_ERROR_UNSUPPORTED;
        }
        if (getPlaneLayouts(drm_format, &planeLayouts) != 0) {
            ALOGE("Failed to get plane layouts for DRM format 0x%x", drm_format);
            return -AIMAPPER_ERROR_UNSUPPORTED;
        }
        for (size_t plane = 0; plane < planeLayouts.size(); plane++) {
            PlaneLayout& planeLayout = planeLayouts[plane];
            planeLayout.offsetInBytes = gralloc_handle_offsets(bufferHandle, plane);
            planeLayout.strideInBytes = gralloc_handle_strides(bufferHandle, plane);
            planeLayout.totalSizeInBytes = gralloc_handle_sizes(bufferHandle, plane);
            planeLayout.widthInSamples = gralloc_handle_width(bufferHandle) / planeLayout.horizontalSubsampling;
            planeLayout.heightInSamples = gralloc_handle_height(bufferHandle) / planeLayout.verticalSubsampling;
        }
        return provide(planeLayouts);
    }
    if constexpr (metadataType == StandardMetadataType::CROP) {
        const uint32_t numPlanes = gralloc_handle_num_planes(bufferHandle);
        const uint32_t w = gralloc_handle_width(bufferHandle);
        const uint32_t h = gralloc_handle_height(bufferHandle);
        std::vector<aidl::android::hardware::graphics::common::Rect> crops;
        for (uint32_t plane = 0; plane < numPlanes; plane++) {
            aidl::android::hardware::graphics::common::Rect crop;
            crop.left = 0;
            crop.top = 0;
            crop.right = w;
            crop.bottom = h;
            crops.push_back(crop);
        }

        return provide(crops);
    }
    if constexpr (metadataType == StandardMetadataType::DATASPACE) {
        std::optional<Dataspace> dataspace;
        if (get_dataspace(bufferHandle, &dataspace)) {
            return -AIMAPPER_ERROR_NO_RESOURCES;
        } else {
            return provide(*dataspace);
        }
    }
    if constexpr (metadataType == StandardMetadataType::BLEND_MODE) {
        std::optional<BlendMode> blend;
        if (get_blend_mode(bufferHandle, &blend)) {
            return -AIMAPPER_ERROR_NO_RESOURCES;
        } else {
            return provide(*blend);
        }
    }
    if constexpr (metadataType == StandardMetadataType::SMPTE2086) {
        std::optional<Smpte2086> smpte;
        if (get_smpte2086(bufferHandle, &smpte)) {
            return -AIMAPPER_ERROR_NO_RESOURCES;
        } else {
            return smpte ? provide(*smpte) : 0;
        }
    }
    if constexpr (metadataType == StandardMetadataType::CTA861_3) {
        std::optional<Cta861_3> cta;
        if (get_cta861_3(bufferHandle, &cta)) {
            return -AIMAPPER_ERROR_NO_RESOURCES;
        } else {
            return cta ? provide(*cta) : 0;
        }
    }
    return -AIMAPPER_ERROR_UNSUPPORTED;
}

AIMapper_Error VivanteMapper::setMetadata(buffer_handle_t _Nonnull buffer,
                                                AIMapper_MetadataType metadataType,
                                                const void* _Nonnull metadata,
                                                size_t metadataSize) {
    ALOGV("setMetadata in");
    VALIDATE_BUFFER_HANDLE(buffer);

    if (!mManager) {
        ALOGE("Failed to setMetadata. Gralloc manager is uninitialized.");
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    // We don't have any vendor-specific metadata, so divert to setStandardMetadata after validating
    // that this is a standard metadata request
    if (isStandardMetadata(metadataType)) {
        return setStandardMetadata(buffer, metadataType.value, metadata, metadataSize);
    }
    return AIMAPPER_ERROR_UNSUPPORTED;
}

AIMapper_Error VivanteMapper::setStandardMetadata(buffer_handle_t _Nonnull bufferHandle,
                                                        int64_t standardTypeRaw,
                                                        const void* _Nonnull metadata,
                                                        size_t metadataSize) {
    ALOGV("setStandardMetadata in");
    VALIDATE_BUFFER_HANDLE(bufferHandle)

    gralloc_handle_t *hnd = gralloc_handle(bufferHandle);

    auto standardType = static_cast<StandardMetadataType>(standardTypeRaw);

    switch (standardType) {
        // Read-only values
        case StandardMetadataType::BUFFER_ID:
        case StandardMetadataType::NAME:
        case StandardMetadataType::WIDTH:
        case StandardMetadataType::HEIGHT:
        case StandardMetadataType::LAYER_COUNT:
        case StandardMetadataType::PIXEL_FORMAT_REQUESTED:
        case StandardMetadataType::USAGE:
            ALOGE("setStandardMetadata bad value");
            return AIMAPPER_ERROR_BAD_VALUE;

        // List supported cases
        case StandardMetadataType::BLEND_MODE:
        case StandardMetadataType::CTA861_3:
        case StandardMetadataType::DATASPACE:
        case StandardMetadataType::SMPTE2086:
            ALOGV("setStandardMetadata supported cases");
            break;

        // Everything else unsupported
        default:
            ALOGE("setStandardMetadata unknown value");
            return AIMAPPER_ERROR_UNSUPPORTED;
    }

    AIMapper_Error status = AIMAPPER_ERROR_UNSUPPORTED;
    auto applier = [&]<StandardMetadataType T>(auto&& value) -> AIMapper_Error {
        return setStandardMetadata<T>(bufferHandle, std::forward<decltype(value)>(value));
    };

    status = applyStandardMetadata(standardType, metadata, metadataSize, applier);

    return status;
}

template <StandardMetadataType TYPE>
AIMapper_Error VivanteMapper::setStandardMetadata(
        buffer_handle_t bufferHandle, typename StandardMetadata<TYPE>::value_type&& value) {
    int ret = 0;
    if constexpr (TYPE == StandardMetadataType::BLEND_MODE) {
        ALOGV("setStandardMetadata BLEND_MODE");
        ret = set_blend_mode(bufferHandle, value);
    }
    if constexpr (TYPE == StandardMetadataType::CTA861_3) {
        ALOGV("setStandardMetadata CTA861_3");
        ret = set_cta861_3(bufferHandle, value);
    }
    if constexpr (TYPE == StandardMetadataType::DATASPACE) {
        ALOGV("setStandardMetadata DATASPACE");
        ret = set_dataspace(bufferHandle, value);
    }
    if constexpr (TYPE == StandardMetadataType::SMPTE2086) {
        ALOGV("setStandardMetadata SMPTE2086");
        ret = set_smpte2086(bufferHandle, value);
    }

    if (ret) {
        ALOGE("setStandardMetadata failed");
        return AIMAPPER_ERROR_NO_RESOURCES;
    }

    // Unsupported metadatas were already filtered before we reached this point
    return AIMAPPER_ERROR_NONE;
}

constexpr AIMapper_MetadataTypeDescription describeStandard(StandardMetadataType type,
                                                            bool isGettable, bool isSettable) {
    return {{STANDARD_METADATA_NAME, static_cast<int64_t>(type)},
            nullptr,
            isGettable,
            isSettable,
            {0}};
}

AIMapper_Error VivanteMapper::listSupportedMetadataTypes(
        const AIMapper_MetadataTypeDescription* _Nullable* _Nonnull outDescriptionList,
        size_t* _Nonnull outNumberOfDescriptions) {

    static constexpr std::array<AIMapper_MetadataTypeDescription, 22> sSupportedMetadaTypes{
            describeStandard(StandardMetadataType::BUFFER_ID, true, false),
            describeStandard(StandardMetadataType::NAME, true, false),
            describeStandard(StandardMetadataType::WIDTH, true, false),
            describeStandard(StandardMetadataType::HEIGHT, true, false),
            describeStandard(StandardMetadataType::LAYER_COUNT, true, false),
            describeStandard(StandardMetadataType::PIXEL_FORMAT_REQUESTED, true, false),
            describeStandard(StandardMetadataType::PIXEL_FORMAT_FOURCC, true, false),
            describeStandard(StandardMetadataType::PIXEL_FORMAT_MODIFIER, true, false),
            describeStandard(StandardMetadataType::USAGE, true, false),
            describeStandard(StandardMetadataType::ALLOCATION_SIZE, true, false),
            describeStandard(StandardMetadataType::PROTECTED_CONTENT, true, false),
            describeStandard(StandardMetadataType::COMPRESSION, true, false),
            describeStandard(StandardMetadataType::INTERLACED, true, false),
            describeStandard(StandardMetadataType::CHROMA_SITING, true, false),
            describeStandard(StandardMetadataType::PLANE_LAYOUTS, true, false),
            describeStandard(StandardMetadataType::CROP, true, false),
            describeStandard(StandardMetadataType::DATASPACE, true, true),
            describeStandard(StandardMetadataType::COMPRESSION, true, false),
            describeStandard(StandardMetadataType::BLEND_MODE, true, true),
            describeStandard(StandardMetadataType::SMPTE2086, true, true),
            describeStandard(StandardMetadataType::CTA861_3, true, true),
            describeStandard(StandardMetadataType::STRIDE, true, false),
    };
    *outDescriptionList = sSupportedMetadaTypes.data();
    *outNumberOfDescriptions = sSupportedMetadaTypes.size();
    return AIMAPPER_ERROR_NONE;
}

AIMapper_Error VivanteMapper::dumpBuffer(
        buffer_handle_t _Nonnull bufferHandle,
        AIMapper_DumpBufferCallback _Nonnull dumpBufferCallback, void* _Null_unspecified context) {

    gralloc_handle_t *hnd = gralloc_handle(bufferHandle);

    // Temp buffer of ~10kb, should be large enough for any of the metadata we want to dump
    std::vector<uint8_t> tempBuffer;
    tempBuffer.resize(10000);
    AIMapper_MetadataType metadataType;
    metadataType.name = STANDARD_METADATA_NAME;

    // Take an instance of the empty StandardMetadat<T> class just to allow auto-deduction
    // to happen as explicit template invocation on lambdas is ugly
    auto dump = [&]<StandardMetadataType T>(StandardMetadata<T>) {
        // Nested templated lambdas! Woo! But the cleanness of the result is worth it
        // The outer lambda exists basically just to capture the StandardMetadataType that's
        // being dumped, as the `provider` parameter of getStandardMetadata only knows
        // the value_type that the enum maps to but not the enum value itself, which we need to
        // construct the `AIMapper_MetadataType` to pass to the dump callback
        auto dumpInner = [&](const typename StandardMetadata<T>::value_type& value) -> int32_t {
            int32_t size =
                    StandardMetadata<T>::value::encode(value, tempBuffer.data(), tempBuffer.size());
            // The initial size should always be large enough, but just in case...
            if (size > tempBuffer.size()) {
                tempBuffer.resize(size * 2);
                size = StandardMetadata<T>::value::encode(value, tempBuffer.data(),
                                                          tempBuffer.size());
            }
            // If the first resize failed _somehow_, just give up. Also don't notify if any
            // errors occurred during encoding.
            if (size >= 0 && size <= tempBuffer.size()) {
                metadataType.value = static_cast<int64_t>(T);
                dumpBufferCallback(context, metadataType, tempBuffer.data(), tempBuffer.size());
            }
            // We don't actually care about the return value in this case, but why not use the
            // real value anyway
            return size;
        };
        getStandardMetadata(bufferHandle, dumpInner, StandardMetadata<T>{});
    };

    // So clean. So pretty.
    dump(StandardMetadata<StandardMetadataType::BUFFER_ID>{});
    dump(StandardMetadata<StandardMetadataType::NAME>{});
    dump(StandardMetadata<StandardMetadataType::WIDTH>{});
    dump(StandardMetadata<StandardMetadataType::HEIGHT>{});
    dump(StandardMetadata<StandardMetadataType::LAYER_COUNT>{});
    dump(StandardMetadata<StandardMetadataType::PIXEL_FORMAT_REQUESTED>{});
    dump(StandardMetadata<StandardMetadataType::PIXEL_FORMAT_FOURCC>{});
    dump(StandardMetadata<StandardMetadataType::PIXEL_FORMAT_MODIFIER>{});
    dump(StandardMetadata<StandardMetadataType::USAGE>{});
    dump(StandardMetadata<StandardMetadataType::ALLOCATION_SIZE>{});
    dump(StandardMetadata<StandardMetadataType::PROTECTED_CONTENT>{});
    dump(StandardMetadata<StandardMetadataType::COMPRESSION>{});
    dump(StandardMetadata<StandardMetadataType::INTERLACED>{});
    dump(StandardMetadata<StandardMetadataType::CHROMA_SITING>{});
    dump(StandardMetadata<StandardMetadataType::PLANE_LAYOUTS>{});
    dump(StandardMetadata<StandardMetadataType::DATASPACE>{});
    dump(StandardMetadata<StandardMetadataType::BLEND_MODE>{});

    return AIMAPPER_ERROR_NONE;
}

AIMapper_Error VivanteMapper::dumpAllBuffers(
        AIMapper_BeginDumpBufferCallback _Nonnull beginDumpBufferCallback,
        AIMapper_DumpBufferCallback _Nonnull dumpBufferCallback, void* _Null_unspecified context) {

    if (!mManager) {
        ALOGE("Failed to dumpAllBuffers. Gralloc manager is uninitialized.");
        return AIMAPPER_ERROR_NO_RESOURCES;
    }

    ALOGI("Gralloc manager buffer number is: %zu", mManager->reserved_region_addrs.size());

    for (const auto &pair : mManager->reserved_region_addrs) {
        buffer_handle_t bufferHandle = (buffer_handle_t) pair.first;

        auto callback = [&](AIMapper_MetadataType type, const std::vector<uint8_t>& buffer) {
            dumpBufferCallback(context, type, buffer.data(), buffer.size());
        };
        beginDumpBufferCallback(context);
		dumpBuffer(bufferHandle, dumpBufferCallback, context);
    }

    return AIMAPPER_ERROR_NONE;
}

AIMapper_Error VivanteMapper::getReservedRegion(buffer_handle_t _Nonnull bufferHandle,
                                                void* _Nullable* _Nonnull outReservedRegion,
                                                 uint64_t* _Nonnull outReservedSize) {
    ALOGV("getReservedRegion in");

    if (!mManager) {
        ALOGE("Failed to getReservedRegion. Gralloc manager is uninitialized.");
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    if (bufferHandle == nullptr) {
        ALOGE("Failed to getReservedRegion. Invalid buffer.");
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    int ret = mManager->get_reserved_region((buffer_handle_t)bufferHandle, outReservedRegion, outReservedSize);
    if (ret) {
        ALOGE("Failed to get_reserved_region");
        *outReservedRegion = nullptr;
        *outReservedSize = 0;
        return AIMAPPER_ERROR_BAD_BUFFER;
    }

    ALOGV("getReservedRegion done with size=%ld", *outReservedSize);
    return AIMAPPER_ERROR_NONE;
}

int32_t VivanteMapper::get_metadata(buffer_handle_t bufferHandle, struct gralloc_metadata **metadata)
{
    void *metadata_addr;
    uint64_t metadata_region_size;

    if (!mManager) {
        ALOGE("Failed to get_metadata. Gralloc manager is uninitialized.");
        return -1;
    }

    int32_t ret = mManager->get_reserved_region(bufferHandle, &metadata_addr, &metadata_region_size);
    if (ret) {
        return ret;
    }

    if (metadata_addr == nullptr) {
        return -1;
    }

    *metadata = reinterpret_cast<struct gralloc_metadata *>(metadata_addr);
    return 0;
}

int32_t VivanteMapper::get_metadata(buffer_handle_t bufferHandle, const struct gralloc_metadata **metadata)
{
    void *metadata_addr;
    uint64_t metadata_region_size;

    if (!mManager) {
        ALOGE("Failed to get_metadata. Gralloc manager is uninitialized.");
        return -1;
    }

    int32_t ret = mManager->get_reserved_region(bufferHandle, &metadata_addr, &metadata_region_size);
    if (ret) {
        return ret;
    }

    if (metadata_addr == nullptr) {
        return -1;
    }

    *metadata = reinterpret_cast<const struct gralloc_metadata *>(metadata_addr);
    return 0;
}

int32_t VivanteMapper::get_name(buffer_handle_t bufferHandle, std::optional<std::string> *name)
{
    const struct gralloc_metadata *metadata;

    int ret = get_metadata(bufferHandle, &metadata);
    if (ret) {
        ALOGE("Failed to get_name: failed to get metadata.");
        return ret;
    }

    *name = metadata->name;
    ALOGV("get_name done");
    return 0;
}

int32_t VivanteMapper::get_blend_mode(buffer_handle_t bufferHandle, std::optional<BlendMode> *blend_mode)
{
    const struct gralloc_metadata *metadata;

    int ret = get_metadata(bufferHandle, &metadata);
    if (ret) {
        ALOGE("Failed to get_blend_mode: failed to get metadata.");
        return ret;
    }

    *blend_mode = metadata->blend_mode;
    ALOGV("get_blend_mode done");
    return 0;
}

int32_t VivanteMapper::set_blend_mode(buffer_handle_t bufferHandle, BlendMode blend_mode)
{
    struct gralloc_metadata *metadata;

    int ret = get_metadata(bufferHandle, &metadata);
    if (ret) {
        ALOGE("Failed to set_blend_mode: failed to get metadata.");
        return ret;
    }

    metadata->blend_mode = blend_mode;
    ALOGV("set_blend_mode done");
    return 0;
}

int32_t VivanteMapper::get_dataspace(buffer_handle_t bufferHandle, std::optional<Dataspace> *dataspace)
{
    const struct gralloc_metadata *metadata;

    int ret = get_metadata(bufferHandle, &metadata);
    if (ret) {
        ALOGE("Failed to get_dataspace: failed to get metadata.");
        return ret;
    }

    ALOGV("Access dataspace from metadata @%p", metadata);
    *dataspace = metadata->dataspace;
    ALOGV("get_dataspace done");

    return 0;
}

int32_t VivanteMapper::set_dataspace(buffer_handle_t bufferHandle, Dataspace dataspace)
{
    struct gralloc_metadata *metadata;

    int ret = get_metadata(bufferHandle, &metadata);
    if (ret) {
        ALOGE("Failed to set_dataspace: failed to get metadata.");
        return ret;
    }

    metadata->dataspace = dataspace;
    ALOGV("set_dataspace done");
    return 0;
}

int32_t VivanteMapper::get_cta861_3(buffer_handle_t bufferHandle, std::optional<Cta861_3> *cta)
{
    const struct gralloc_metadata *metadata;

    int ret = get_metadata(bufferHandle, &metadata);
    if (ret) {
        ALOGE("Failed to get_cta861_3: failed to get metadata.");
        return ret;
    }

    *cta = metadata->cta861_3;
    ALOGV("get_cta861_3 done");
    return 0;
}

int32_t VivanteMapper::set_cta861_3(buffer_handle_t bufferHandle, std::optional<Cta861_3> cta)
{
    struct gralloc_metadata *metadata;

    int ret = get_metadata(bufferHandle, &metadata);
    if (ret) {
        ALOGE("Failed to set_cta861_3: failed to get metadata.");
        return ret;
    }

    metadata->cta861_3 = cta;
    ALOGV("set_cta861_3 done");
    return 0;
}

int32_t VivanteMapper::get_smpte2086(buffer_handle_t bufferHandle, std::optional<Smpte2086> *smpte)
{
    const struct gralloc_metadata *metadata;

    int ret = get_metadata(bufferHandle, &metadata);
    if (ret) {
        ALOGE("Failed to get_smpte2086: failed to get metadata.");
        return ret;
    }

    *smpte = metadata->smpte2086;
    ALOGV("get_smpte2086 done");
    return 0;
}

int32_t VivanteMapper::set_smpte2086(buffer_handle_t bufferHandle, std::optional<Smpte2086> smpte)
{
    struct gralloc_metadata *metadata;

    int ret = get_metadata(bufferHandle, &metadata);
    if (ret) {
        ALOGE("Failed to set_cta861_3: failed to get metadata.");
        return ret;
    }

    metadata->smpte2086 = smpte;
    ALOGV("set_smpte2086 done");
    return 0;
}

extern "C" uint32_t ANDROID_HAL_MAPPER_VERSION = AIMAPPER_VERSION_5;

extern "C" AIMapper_Error AIMapper_loadIMapper(AIMapper* _Nullable* _Nonnull outImplementation) {
    static vendor::mapper::IMapperProvider<VivanteMapper> provider;
    return provider.load(outImplementation);
}
