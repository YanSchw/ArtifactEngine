#pragma once
#include "Asset.h"
#include "Common/Types.h"
#include "Object/Pointer.h"
#include "Rendering/Image.h"
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
    virtual String GetDisplayName() const override { return IsSubAsset() ? Asset::GetDisplayName() : DisplayNameFromPath(m_TexturePath); }
    virtual String GetSourcePath() const override { return m_TexturePath; }
    virtual void SetSourcePath(const String& InPath) override { m_TexturePath = InPath; }
    Texture* GetTexture() const;

    void SetGenerateMips(bool InGenerateMips) { m_GenerateMips = InGenerateMips; }
    void Reimport();

private:
    bool ImportSource(Array<byte>& OutPixels, uint32_t& OutWidth, uint32_t& OutHeight, ImageFormat& OutFormat) const;

    SharedObjectPtr<Texture> m_Texture = nullptr;

    PROPERTY()
    String m_TexturePath;

    PROPERTY()
    bool m_GenerateMips = true;

    PROPERTY()
    bool m_FlipVertically = false;
};