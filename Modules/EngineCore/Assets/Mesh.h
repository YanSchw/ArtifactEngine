#pragma once
#include "Asset.h"
#include "Object/Pointer.h"
#include "Mesh.gen.h"

class VertexBuffer;

class Mesh : public Asset {
public:
    ARTIFACT_CLASS();
    Mesh();
    virtual ~Mesh() = default;

protected:
    virtual void Load() override;
    virtual void Unload() override;
    virtual void Cook(class ChunkedBinary& OutChunkedBinary) override;

public:
    virtual bool IsLoaded() const override;
    virtual String GetDisplayName() const override { return DisplayNameFromPath(m_MeshPath); }
    VertexBuffer* GetVertexBuffer() const;

private:
    SharedObjectPtr<VertexBuffer> m_VertexBuffer = nullptr;

    PROPERTY()
    String m_MeshPath;
};