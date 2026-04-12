#pragma once
#include "Core/Object.h"
#include "Core/Pointer.h"
#include "Common/Types.h"
#include "Image.h"
#include "Sampler.gen.h"

enum class FilterMode {
    Nearest,
    Linear
};

enum class AddressMode {
    Repeat,
    Clamp,
    Mirror
};

struct SamplerDesc {
    FilterMode MinFilter = FilterMode::Linear;
    FilterMode MagFilter = FilterMode::Linear;

    AddressMode AddressU = AddressMode::Repeat;
    AddressMode AddressV = AddressMode::Repeat;
    AddressMode AddressW = AddressMode::Repeat;
};

class Sampler : public Object {
public:
    ARTIFACT_CLASS();
    virtual ~Sampler() = default;

    static SharedObjectPtr<Sampler> Create(const SamplerDesc& InSamplerDesc);
};