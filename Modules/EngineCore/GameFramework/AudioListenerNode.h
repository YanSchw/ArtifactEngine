#pragma once
#include "Node3D.h"
#include "AudioListenerNode.gen.h"

class AudioListenerNode : public Node3D {
public:
    ARTIFACT_CLASS();

    virtual void BeginPlay() override;
    virtual void EndPlay() override;

    FUNCTION()
    void Activate();
    FUNCTION(Pure)
    bool IsActive() const;
};
