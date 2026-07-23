#include "EditorIcons.h"

VectorImage* EditorIcons::GetNodeIcon(const Class& InClass) {
    static Map<String, VectorImage*> s_NodeIcons;
    AssetManager& assets = AssetManager::Get();
    if (s_NodeIcons.Size() == 0) {
        s_NodeIcons["Node"] = Get("b1c2d3e4-0001-4a00-9000-000000000001");
        s_NodeIcons["Node3D"] = Get("b1c2d3e4-0009-4a00-9000-000000000014");
        s_NodeIcons["StaticMeshNode"] = Get("b1c2d3e4-000f-4a00-9000-00000000000f");
        s_NodeIcons["Component"] = Get("b1c2d3e4-0009-4a00-9000-000000000012");
    }

    return s_NodeIcons.ContainsKey(InClass.Name) ? s_NodeIcons[InClass.Name] : GetNodeIcon(InClass.GetParentClass());
}
