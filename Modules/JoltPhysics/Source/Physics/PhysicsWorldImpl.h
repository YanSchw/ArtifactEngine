#pragma once
#include "JoltContactListener.h"
#include "JoltLayers.h"
#include <Jolt/Physics/PhysicsSystem.h>

struct PhysicsWorldImpl {
    BroadPhaseLayerMap BroadPhaseLayers;
    ObjectVsBroadPhaseLayerFilter BroadPhaseFilter;
    ObjectLayerPairFilter LayerFilter;
    JoltContactListener ContactListener;
    JPH::PhysicsSystem System;
};
