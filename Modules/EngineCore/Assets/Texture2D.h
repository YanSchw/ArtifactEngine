#pragma once
#include "Asset.h"
#include "Object/Pointer.h"
#include "Texture2D.gen.h"

class Texture;

class Texture2D : public Asset {
public:
    ARTIFACT_CLASS();
    Texture2D();
    virtual ~Texture2D() = default;

protected:
    virtual void Load() override;
    virtual void Unload() override;

public:
    virtual bool IsLoaded() const override;
    Texture* GetTexture() const;

private:
    SharedObjectPtr<Texture> m_Texture = nullptr;

    PROPERTY()
    String m_TexturePath;
};