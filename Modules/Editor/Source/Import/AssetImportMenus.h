#pragma once
#include "Object/Object.h"
#include "Common/Array.h"
#include "AssetImportMenus.gen.h"

class Asset;

/** One way of importing a kind of file. Subclassing is the registration.
 *  When several menus take the same file the user picks one in the import dialog. */
class AssetImportMenu : public Object {
public:
    ARTIFACT_CLASS();

    virtual String GetName() const = 0;
    virtual Class GetAssetClass() const = 0;
    virtual bool CanImport(const String& InFilePath) const = 0;
    /** Ordered first in the dialog. */
    virtual int32_t GetPriority() const { return 0; }
    /** Every file that has to come along into the project for the source to import there again. */
    virtual Array<String> GetSourceFiles(const String& InFilePath) const { return { InFilePath }; }
    /** Runs once the new asset is registered and points at its source. */
    virtual void OnImported(Asset& InAsset) const { (void)InAsset; }

    static Array<AssetImportMenu*> FindFor(const String& InFilePath);
};

class MeshImportMenu : public AssetImportMenu {
public:
    ARTIFACT_CLASS();

    virtual String GetName() const override { return "Static Mesh"; }
    virtual Class GetAssetClass() const override;
    virtual bool CanImport(const String& InFilePath) const override;
    virtual Array<String> GetSourceFiles(const String& InFilePath) const override;
};

class CollectionImportMenu : public AssetImportMenu {
public:
    ARTIFACT_CLASS();

    virtual String GetName() const override { return "Collection"; }
    virtual Class GetAssetClass() const override;
    virtual bool CanImport(const String& InFilePath) const override;
    virtual Array<String> GetSourceFiles(const String& InFilePath) const override;
    virtual int32_t GetPriority() const override { return 1; }
    virtual void OnImported(Asset& InAsset) const override;
};

class Texture2DImportMenu : public AssetImportMenu {
public:
    ARTIFACT_CLASS();

    virtual String GetName() const override { return "Texture"; }
    virtual Class GetAssetClass() const override;
    virtual bool CanImport(const String& InFilePath) const override;
};

class VectorImageImportMenu : public AssetImportMenu {
public:
    ARTIFACT_CLASS();

    virtual String GetName() const override { return "Vector Image"; }
    virtual Class GetAssetClass() const override;
    virtual bool CanImport(const String& InFilePath) const override;
};

class FontImportMenu : public AssetImportMenu {
public:
    ARTIFACT_CLASS();

    virtual String GetName() const override { return "Font"; }
    virtual Class GetAssetClass() const override;
    virtual bool CanImport(const String& InFilePath) const override;
};

class AudioClipImportMenu : public AssetImportMenu {
public:
    ARTIFACT_CLASS();

    virtual String GetName() const override { return "Audio Clip"; }
    virtual Class GetAssetClass() const override;
    virtual bool CanImport(const String& InFilePath) const override;
};
