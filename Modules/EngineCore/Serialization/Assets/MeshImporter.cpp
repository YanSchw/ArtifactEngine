#include "MeshImporter.h"

ImportedMesh ImportedMesh::Combine(const Array<ImportedMesh>& InMeshes, const String& InName) {
    Array<String> slotNames;
    Array<Array<uint32_t>> slotIndices;

    ImportedMesh combined;
    combined.Name = InName;
    for (const ImportedMesh& mesh : InMeshes) {
        const uint32_t baseVertex = (uint32_t)combined.Vertices.Size();
        combined.Vertices += mesh.Vertices;

        for (const SubMesh& subMesh : mesh.SubMeshes) {
            int32_t slot = slotNames.IndexOf(subMesh.Name);
            if (slot < 0) {
                slot = slotNames.Size();
                slotNames.Add(subMesh.Name);
                slotIndices.Add(Array<uint32_t>());
            }
            for (uint32_t i = subMesh.FirstIndex; i < subMesh.FirstIndex + subMesh.IndexCount; i++) {
                slotIndices[slot].Add(baseVertex + mesh.Indices[i]);
            }
        }
    }

    for (int32_t slot = 0; slot < slotNames.Size(); slot++) {
        combined.SubMeshes.Add({ slotNames[slot], (uint32_t)combined.Indices.Size(), (uint32_t)slotIndices[slot].Size() });
        combined.Indices += slotIndices[slot];
    }
    return combined;
}
