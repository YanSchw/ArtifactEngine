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
    static VectorImage* UseSelected()   { return Get("c605a931-fe32-4afc-b054-ff46bc5d9ecf"); }
    static VectorImage* Search()        { return Get("fcc09193-44bc-4f87-9e1e-f77546713c07"); }
    static VectorImage* Play()          { return Get("f24d1510-5091-462e-be7d-0a9cc35a711b"); }
    static VectorImage* Simulate()      { return Get("1ec662cf-ef29-440d-94b2-7980f6b2ba7d"); }
    static VectorImage* Stop()          { return Get("0457f0fb-4131-4edf-814f-13986bb58f97"); }
    static VectorImage* Eject()         { return Get("b96bbcd6-154c-4705-a371-db1c7d229b3d"); }
    static VectorImage* Possess()       { return Get("d273a65f-36fb-4737-b30e-85549591eec9"); }
    static VectorImage* Material()      { return Get("64a96b51-290f-4883-a5fd-b1e7707b4fd8"); }
    static VectorImage* BoxShape()      { return Get("8b08ff80-2548-494d-b6dd-ef63ae979485"); }
    static VectorImage* SphereShape()   { return Get("09560261-b21c-45b0-952a-6c80331d2fe4"); }
    static VectorImage* CapsuleShape()  { return Get("ca8c7b74-272d-41a5-bde0-3b4496ff5dc9"); }
    static VectorImage* CylinderShape() { return Get("1e9a2e01-2f2f-4873-b28a-947f2f69c2c3"); }
    static VectorImage* RigidBody()     { return Get("d06d74b9-dfa5-4126-981a-4b9872ab540a"); }
    static VectorImage* Character()     { return Get("37661651-475f-4abb-9aab-8d05e8da1fc0"); }
    static VectorImage* Animation()     { return Get("3f0e57a2-1c4b-4d90-9d0e-7a1b2c3d4e5f"); }
    static VectorImage* Record()        { return Get("5b2c8e14-9d33-4f61-8a72-6c0d51e9b3a4"); }
    static VectorImage* Eye()           { return Get("b1c2d3e4-0004-4a00-9000-000000000004"); }
    static VectorImage* Save()          { return Get("e173f6e2-fc21-4933-b11f-0e1bff34ece1"); }
    static VectorImage* Select()        { return Get("fc8a90f0-1426-4ddb-837e-732882527a91"); }
    static VectorImage* Move()          { return Get("fa00bfd5-a7a9-4e03-9da7-cac8c9276031"); }
    static VectorImage* Rotate()        { return Get("8bcdae6a-a930-49f3-9bb4-77027d91bccc"); }
    static VectorImage* Scale()         { return Get("6dbc20bf-42b5-41f0-b33c-f53eb22f0cdf"); }
    static VectorImage* RectTool()      { return Get("029e2b33-c934-4392-928b-7e27d38f0bf9"); }
    static VectorImage* WorldSpace()    { return Get("c25d2ba0-a8db-4a5f-a315-5f366fbc8ad0"); }
    static VectorImage* LocalSpace()    { return Get("8cc2ddce-1fc9-4375-b4aa-b6eaed517caa"); }

    static VectorImage* GetNodeIcon(const Class& InClass);
    static VectorImage* GetAssetIcon(const Class& InClass);
    static Vec4 GetAssetColor(const Class& InClass);

    static void Paint(UIDrawList& OutDrawList, VectorImage* InIcon, const UIRectF& InRect,
                      const Vec4& InTint, const Mat4& InTransform);
};
