#pragma once
#include "Assets/AssetManager.h"
#include "Assets/VectorImage.h"
#include "Common/UUID.h"

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
};
