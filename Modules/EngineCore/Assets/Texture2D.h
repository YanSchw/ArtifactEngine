#pragma once
#include "Asset.h"
#include "Common/Types.h"
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
    virtual void Cook(class ChunkedBinary& OutChunkedBinary) override;

public:
    virtual bool IsLoaded() const override;
    virtual String GetDisplayName() const override { return DisplayNameFromPath(m_TexturePath); }
    Texture* GetTexture() const;

private:
    bool ImportSource(Array<byte>& OutPixels, uint32_t& OutWidth, uint32_t& OutHeight) const;

    SharedObjectPtr<Texture> m_Texture = nullptr;

    PROPERTY()
    String m_TexturePath;
};