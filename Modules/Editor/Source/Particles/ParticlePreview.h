#pragma once
#include "Common/Types.h"

class World;
class ParticleSystemAsset;
class ParticleSystemEmitterNode;

class ParticlePreview {
public:
    static void Tick(World& InWorld, float InDeltaTime);
    static void DrawShapes(World& InWorld, const ParticleSystemAsset& InSystem, const Mat4& InTransform);

private:
    static void Sync(ParticleSystemEmitterNode& InEmitter);
};
