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
    static VectorImage* ArtifactLogo()  { return Get("349fb27d-d2ef-4f75-8529-61d9fc25a4d3"); }
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
    static VectorImage* Collection()    { return Get("c0d8525b-a6d4-4716-8bb7-b501e58a4519"); }
    static VectorImage* Outliner()      { return Get("b1c2d3e4-0015-4a00-9000-000000000015"); }
    static VectorImage* Details()       { return Get("b1c2d3e4-0016-4a00-9000-000000000016"); }
    static VectorImage* Viewport()      { return Get("b1c2d3e4-0017-4a00-9000-000000000017"); }
    static VectorImage* GraphEditor()   { return Get("b1c2d3e4-0018-4a00-9000-000000000018"); }
    static VectorImage* Message()       { return Get("b1c2d3e4-0019-4a00-9000-000000000019"); }
    static VectorImage* Close()         { return Get("b1c2d3e4-001a-4a00-9000-00000000001a"); }
    static VectorImage* Reset()         { return Get("b1c2d3e4-0006-4a00-9000-000000000006"); }
    static VectorImage* Settings()      { return Get("bad42a9c-6ed5-4480-87b5-80327decc45d"); }
    static VectorImage* UseSelected()   { return Get("c605a931-fe32-4afc-b054-ff46bc5d9ecf"); }
    static VectorImage* Search()        { return Get("fcc09193-44bc-4f87-9e1e-f77546713c07"); }
    static VectorImage* Play()          { return Get("f24d1510-5091-462e-be7d-0a9cc35a711b"); }
    static VectorImage* Simulate()      { return Get("1ec662cf-ef29-440d-94b2-7980f6b2ba7d"); }
    static VectorImage* Stop()          { return Get("0457f0fb-4131-4edf-814f-13986bb58f97"); }
    static VectorImage* Pause()         { return Get("1e8c9564-65a0-427b-88b9-005157ff03dc"); }
    static VectorImage* Loop()          { return Get("64ce3236-65ee-4ee9-89d4-27bcb4e46089"); }
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
    static VectorImage* InputMapping()  { return Get("3c00f557-6c65-4ca3-b35d-6bfa9c50fd2f"); }
    static VectorImage* Audio()         { return Get("87b011c9-7a52-4b36-880b-f817fdaaa9ca"); }
    static VectorImage* AudioListener() { return Get("a012916e-b923-43e3-bb3d-e85d3d191de2"); }
    static VectorImage* PointLight()    { return Get("1c3c5fc9-c4ae-4b78-99ac-a84fe1b5feb5"); }
    static VectorImage* DirectionalLight() { return Get("b8fd4487-6c48-48a7-bb71-bf8645260b58"); }
    static VectorImage* PostProcessVolume() { return Get("020164d8-0b57-4f63-b4b1-d99a99d668b3"); }
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
    static VectorImage* Perspective()   { return Get("174f84e6-e3bc-431b-8f3c-152cab720fde"); }
    static VectorImage* Orthographic()  { return Get("5caf61cf-70e6-4d1e-811e-64b79c134e8e"); }
    static VectorImage* Lit()           { return Get("3a824297-6560-4cc0-9d68-3c55b640d21b"); }
    static VectorImage* BufferView()    { return Get("27323ef9-9e7d-4712-816f-1613c2d7bf48"); }
    static VectorImage* Stats()         { return Get("ef2ce730-dc8f-40bf-a5ab-7cd2b4b07e9e"); }
    static VectorImage* Gizmos()        { return Get("20d37e29-0971-43cf-ab35-5d0535719822"); }

    static VectorImage* GetNodeIcon(const Class& InClass);
    static VectorImage* GetAssetIcon(const Class& InClass);
    static Vec4 GetAssetColor(const Class& InClass);

    static void Paint(UIDrawList& OutDrawList, VectorImage* InIcon, const UIRectF& InRect,
                      const Vec4& InTint, const Mat4& InTransform);
};
