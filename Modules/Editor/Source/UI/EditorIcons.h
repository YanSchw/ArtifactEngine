#pragma once
#include "Assets/AssetManager.h"
#include "Assets/VectorImage.h"
#include "Common/UUID.h"
#include "GameFramework/UILayout.h"

class UIDrawList;

/** The editor's built-in vector icons, resolved from the AssetManager by their fixed content UUIDs. */
class EditorIcons {
public:
    static VectorImage* Get(const char* InUuid) {
        return AssetManager::Get().GetAsset<VectorImage>(UUID::FromString(InUuid));
    }
    static VectorImage* Node()          { return Get("b1c2d3e4-0001-4a00-9000-000000000001"); }
    static VectorImage* ArrowDown()     { return Get("b1c2d3e4-0002-4a00-9000-000000000002"); }
    static VectorImage* ArrowRight()    { return Get("b1c2d3e4-0003-4a00-9000-000000000003"); }
    static VectorImage* ArrowUp()       { return Get("b1c2d3e4-0013-4a00-9000-000000000013"); }
    static VectorImage* ArrowLeft()     { return Get("b1c2d3e4-0014-4a00-9000-000000000014"); }
    static VectorImage* Level()         { return Get("b1c2d3e4-0008-4a00-9000-000000000008"); }
    static VectorImage* Document()      { return Get("b1c2d3e4-0009-4a00-9000-000000000009"); }
    static VectorImage* ContentDrawer() { return Get("b1c2d3e4-000a-4a00-9000-00000000000a"); }
    static VectorImage* Console()       { return Get("b1c2d3e4-000b-4a00-9000-00000000000b"); }
    static VectorImage* Warning()       { return Get("b1c2d3e4-000c-4a00-9000-00000000000c"); }
    static VectorImage* Error()         { return Get("b1c2d3e4-000d-4a00-9000-00000000000d"); }
    static VectorImage* Folder()        { return Get("b1c2d3e4-000e-4a00-9000-00000000000e"); }
    static VectorImage* Mesh()          { return Get("b1c2d3e4-000f-4a00-9000-00000000000f"); }
    static VectorImage* Texture()       { return Get("b1c2d3e4-0010-4a00-9000-000000000010"); }
    static VectorImage* Font()          { return Get("b1c2d3e4-0011-4a00-9000-000000000011"); }
    static VectorImage* Asset()         { return Get("b1c2d3e4-0012-4a00-9000-000000000012"); }
    static VectorImage* Outliner()      { return Get("b1c2d3e4-0015-4a00-9000-000000000015"); }
    static VectorImage* Details()       { return Get("b1c2d3e4-0016-4a00-9000-000000000016"); }
    static VectorImage* Viewport()      { return Get("b1c2d3e4-0017-4a00-9000-000000000017"); }
    static VectorImage* GraphEditor()   { return Get("b1c2d3e4-0018-4a00-9000-000000000018"); }
    static VectorImage* Message()       { return Get("b1c2d3e4-0019-4a00-9000-000000000019"); }
    static VectorImage* Close()         { return Get("b1c2d3e4-001a-4a00-9000-00000000001a"); }
    static VectorImage* Play()          { return Get("f24d1510-5091-462e-be7d-0a9cc35a711b"); }
    static VectorImage* Simulate()      { return Get("1ec662cf-ef29-440d-94b2-7980f6b2ba7d"); }
    static VectorImage* Stop()          { return Get("0457f0fb-4131-4edf-814f-13986bb58f97"); }
    static VectorImage* Eject()         { return Get("b96bbcd6-154c-4705-a371-db1c7d229b3d"); }
    static VectorImage* Possess()       { return Get("d273a65f-36fb-4737-b30e-85549591eec9"); }

    static VectorImage* GetNodeIcon(const Class& InClass);
    static VectorImage* GetAssetIcon(const Class& InClass);
    static Vec4 GetAssetColor(const Class& InClass);

    static void Paint(UIDrawList& OutDrawList, VectorImage* InIcon, const UIRectF& InRect,
                      const Vec4& InTint, const Mat4& InTransform);
};
