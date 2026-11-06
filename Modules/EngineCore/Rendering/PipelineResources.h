#pragma once
#include "CoreMinimal.h"

#include <tuple>
#include <utility>

using SampledImageBinding = std::tuple<uint32_t, SharedObjectPtr<class ImageView>, SharedObjectPtr<class Sampler>>;
using StorageImageBinding = std::pair<uint32_t, SharedObjectPtr<class ImageView>>;

/** The shader and every resource it reads or writes, shared by graphics and compute pipelines.
 *  Buffers bind at the slot they were created with. Storage images need ImageUsage::Storage. */
struct PipelineResources {
    SharedObjectPtr<class Shader> Shader;
    Array<SharedObjectPtr<class ShaderBuffer>> Buffers;
    Array<SampledImageBinding> ImageBindings;
    Array<StorageImageBinding> StorageImages;
};