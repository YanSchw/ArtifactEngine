#pragma once
#include "Object/Object.h"
#include <memory>

enum class ScriptFieldKind : int32_t { Float, Double, Int32, Bool, String, Vec2, Vec3, Vec4, Color, Class };

struct ScriptField {
    String Name;
    ScriptFieldKind Kind;
    uint32_t Offset;
};

struct ScriptClassInfo {
    String Name;
    Class NativeBase;
    Array<ScriptField> Fields;
    uint32_t StorageOffset = 0;
    uint32_t StorageSize = 0;
    bool Ticks = false;

    void ConstructStorage(char* InStorage) const;
    void DestructStorage(char* InStorage) const;
};

struct ScriptInstance {
    std::shared_ptr<const ScriptClassInfo> Info;
    uint32_t Capacity = 0;
    bool Stale = false;
};

inline constexpr uint32_t ScriptStorageReserve = 1024;

enum class ScriptCall { BeginPlay, EndPlay, WorldUpdate };

namespace ScriptBridge {
    void Construct(Object& InObject, ScriptInstance& InInstance);
    void Destruct(Object& InObject, ScriptInstance& InInstance);
    Class ClassOf(const ScriptInstance& InInstance);

    void CreateManaged(Object& InObject, const ScriptClassInfo& InInfo, char* InStorage);
    void DestroyManaged(Object& InObject);
    void Call(Object& InObject, ScriptCall InCall, float InDeltaTime = 0.0f);
}

namespace ScriptRegistry {
    struct FieldDesc {
        String Name;
        ScriptFieldKind Kind;
    };

    struct MigrationResult {
        int32_t Migrated = 0;
        int32_t Stale = 0;
    };

    void Clear();
    const ScriptClassInfo* Register(const String& InName, const String& InParent, const String& InModule, bool InTicks,
                                    const Array<FieldDesc>& InFields);
    int32_t GetClassCount();
    MigrationResult Migrate();
}
