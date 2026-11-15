#pragma once
#include "Asset.h"
#include "DataAsset.gen.h"

class DataAsset : public Asset {
public:
    ARTIFACT_CLASS();

protected:
    DataAsset() = default;

    virtual void Load() override {}
    virtual void Unload() override {}
};
