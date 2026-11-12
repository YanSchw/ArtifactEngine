#include "ScriptClass.h"
#include "ScriptProxy.h"
#include "Core/Log.h"
#include "GameFramework/Node3D.h"
#include "Object/Property.h"
#include <cstring>
#include <unordered_map>

struct ScriptBase {
    Class NativeClass;
    uint32_t ProxySize;
    Object* (*Allocate)(std::shared_ptr<const ScriptClassInfo>);
};

static Map<String, std::shared_ptr<const ScriptClassInfo>> s_Classes;
static std::unordered_map<Object*, ScriptInstance*> s_Instances;

static const Array<ScriptBase>& GetScriptBases() {
    static const Array<ScriptBase> s_Bases = {
        { Object::StaticClass(), sizeof(ScriptObjectProxy), &AllocateScriptProxy<ScriptObjectProxy> },
        { Node::StaticClass(), sizeof(ScriptNodeProxy<Node>), &AllocateScriptProxy<ScriptNodeProxy<Node>> },
        { Node3D::StaticClass(), sizeof(ScriptNodeProxy<Node3D>), &AllocateScriptProxy<ScriptNodeProxy<Node3D>> },
    };
    return s_Bases;
}

static const ScriptBase* FindScriptBase(const Class& InClass) {
    for (const ScriptBase& base : GetScriptBases()) {
        if (base.NativeClass == InClass) {
            return &base;
        }
    }
    return nullptr;
}

static uint32_t AlignUp(uint32_t InValue, uint32_t InAlignment) {
    return (InValue + InAlignment - 1) / InAlignment * InAlignment;
}

static uint32_t SizeOf(ScriptFieldKind InKind) {
    switch (InKind) {
        case ScriptFieldKind::Float: return sizeof(float);
        case ScriptFieldKind::Double: return sizeof(double);
        case ScriptFieldKind::Int32: return sizeof(int32_t);
        case ScriptFieldKind::Bool: return sizeof(bool);
        case ScriptFieldKind::String: return sizeof(String);
        case ScriptFieldKind::Vec2: return sizeof(Vec2);
        case ScriptFieldKind::Vec3: return sizeof(Vec3);
        case ScriptFieldKind::Vec4: return sizeof(Vec4);
        case ScriptFieldKind::Color: return sizeof(Color);
    }
    return 0;
}

static uint32_t AlignOf(ScriptFieldKind InKind) {
    switch (InKind) {
        case ScriptFieldKind::Double: return alignof(double);
        case ScriptFieldKind::Bool: return alignof(bool);
        case ScriptFieldKind::String: return alignof(String);
        default: return alignof(float);
    }
}

static Property* MakeProperty(const String& InName, ScriptFieldKind InKind, uint64_t InOffset) {
    switch (InKind) {
        case ScriptFieldKind::Float: return new FloatProperty(InName, InOffset, false);
        case ScriptFieldKind::Double: return new FloatProperty(InName, InOffset, true);
        case ScriptFieldKind::Int32: return new IntProperty(InName, InOffset, false, 32);
        case ScriptFieldKind::Bool: return new BoolProperty(InName, InOffset);
        case ScriptFieldKind::String: return new StringProperty(InName, InOffset);
        case ScriptFieldKind::Vec2: return new StructProperty(InName, InOffset, "Vec2");
        case ScriptFieldKind::Vec3: return new StructProperty(InName, InOffset, "Vec3");
        case ScriptFieldKind::Vec4: return new StructProperty(InName, InOffset, "Vec4");
        case ScriptFieldKind::Color: return new StructProperty(InName, InOffset, "Color");
    }
    return nullptr;
}

static void CopyField(ScriptFieldKind InKind, char* OutValue, const char* InValue) {
    if (InKind == ScriptFieldKind::String) {
        *reinterpret_cast<String*>(OutValue) = *reinterpret_cast<const String*>(InValue);
    } else {
        std::memcpy(OutValue, InValue, SizeOf(InKind));
    }
}

static char* StorageOf(Object& InObject, const ScriptClassInfo& InInfo) {
    return reinterpret_cast<char*>(&InObject) + InInfo.StorageOffset;
}

void ScriptClassInfo::ConstructStorage(char* InStorage) const {
    std::memset(InStorage, 0, StorageSize);
    for (const ScriptField& field : Fields) {
        if (field.Kind == ScriptFieldKind::String) {
            ::new (InStorage + field.Offset) String();
        }
    }
}

void ScriptClassInfo::DestructStorage(char* InStorage) const {
    for (const ScriptField& field : Fields) {
        if (field.Kind == ScriptFieldKind::String) {
            reinterpret_cast<String*>(InStorage + field.Offset)->~String();
        }
    }
}

void ScriptBridge::Construct(Object& InObject, ScriptInstance& InInstance) {
    char* storage = StorageOf(InObject, *InInstance.Info);
    InInstance.Info->ConstructStorage(storage);
    s_Instances[&InObject] = &InInstance;
    CreateManaged(InObject, *InInstance.Info, storage);
}

void ScriptBridge::Destruct(Object& InObject, ScriptInstance& InInstance) {
    DestroyManaged(InObject);
    s_Instances.erase(&InObject);
    InInstance.Info->DestructStorage(StorageOf(InObject, *InInstance.Info));
}

Class ScriptBridge::ClassOf(const ScriptInstance& InInstance) {
    return InInstance.Stale ? InInstance.Info->NativeBase : Class(InInstance.Info->Name);
}

void ScriptRegistry::Clear() {
    s_Classes.Clear();
}

int32_t ScriptRegistry::GetClassCount() {
    return (int32_t)s_Classes.Size();
}

const ScriptClassInfo* ScriptRegistry::Register(const String& InName, const String& InParent, const String& InModule, bool InTicks,
                                                const Array<FieldDesc>& InFields) {
    std::shared_ptr<ScriptClassInfo> info = std::make_shared<ScriptClassInfo>();
    info->Name = InName;
    info->Ticks = InTicks;
    if (std::shared_ptr<const ScriptClassInfo>* parent = s_Classes.Find(InParent)) {
        info->NativeBase = (*parent)->NativeBase;
        info->Fields = (*parent)->Fields;
        info->StorageSize = (*parent)->StorageSize;
        info->Ticks = info->Ticks || (*parent)->Ticks;
    } else {
        info->NativeBase = Class(InParent);
    }

    const ScriptBase* base = FindScriptBase(info->NativeBase);
    if (!base) {
        AE_ERROR("Script class '{0}' derives from '{1}', which scripts cannot extend yet", InName, InParent);
        return nullptr;
    }
    info->StorageOffset = AlignUp(base->ProxySize, 16);

    Array<Property*> properties;
    for (const FieldDesc& desc : InFields) {
        const uint32_t offset = AlignUp(info->StorageSize, AlignOf(desc.Kind));
        info->StorageSize = offset + SizeOf(desc.Kind);
        info->Fields.Add({ desc.Name, desc.Kind, offset });
        properties.Add(MakeProperty(desc.Name, desc.Kind, info->StorageOffset + offset));
    }
    info->StorageSize = AlignUp(std::max(info->StorageSize, 1u), 16);

    s_Classes[InName] = info;
    Property::ReplaceTypeProperties(InName, properties);
    Object* (*allocate)(std::shared_ptr<const ScriptClassInfo>) = base->Allocate;
    Class::RegisterRuntimeClass(InName, Class(InParent), InModule, [InName, allocate]() -> Object* {
        std::shared_ptr<const ScriptClassInfo>* current = s_Classes.Find(InName);
        return current ? allocate(*current) : nullptr;
    });
    return info.get();
}

static const ScriptField* FindField(const ScriptClassInfo& InInfo, const ScriptField& InLike) {
    for (const ScriptField& field : InInfo.Fields) {
        if (field.Name == InLike.Name && field.Kind == InLike.Kind) {
            return &field;
        }
    }
    return nullptr;
}

static void MigrateInstance(Object& InObject, ScriptInstance& InInstance, std::shared_ptr<const ScriptClassInfo> InNext) {
    const std::shared_ptr<const ScriptClassInfo> previous = InInstance.Info;
    char* storage = StorageOf(InObject, *previous);

    std::unique_ptr<char[]> kept = std::make_unique<char[]>(InNext->StorageSize);
    InNext->ConstructStorage(kept.get());
    Array<const ScriptField*> carried;
    for (const ScriptField& field : InNext->Fields) {
        if (const ScriptField* old = FindField(*previous, field)) {
            CopyField(field.Kind, kept.get() + field.Offset, storage + old->Offset);
            carried.Add(&field);
        }
    }

    previous->DestructStorage(storage);
    InInstance.Info = InNext;
    InNext->ConstructStorage(storage);
    ScriptBridge::CreateManaged(InObject, *InNext, storage);
    for (const ScriptField* field : carried) {
        CopyField(field->Kind, storage + field->Offset, kept.get() + field->Offset);
    }
    InNext->DestructStorage(kept.get());

    if (InNext->Ticks && !previous->Ticks) {
        if (Node* node = Cast<Node>(&InObject)) {
            node->SetUpdateFlag(UpdateFlag::WorldUpdate);
        }
    }
}

ScriptRegistry::MigrationResult ScriptRegistry::Migrate() {
    MigrationResult result;
    const std::unordered_map<Object*, ScriptInstance*> instances = s_Instances;
    for (const auto& [object, instance] : instances) {
        if (instance->Stale) {
            continue;
        }
        std::shared_ptr<const ScriptClassInfo>* next = s_Classes.Find(instance->Info->Name);
        if (!next || (*next)->NativeBase != instance->Info->NativeBase || (*next)->StorageSize > instance->Capacity) {
            instance->Stale = true;
            result.Stale++;
            continue;
        }
        MigrateInstance(*object, *instance, *next);
        result.Migrated++;
    }
    return result;
}
