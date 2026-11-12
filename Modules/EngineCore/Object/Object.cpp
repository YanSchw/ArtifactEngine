#include "Object.h"
#include "Pointer.h"
#include "Common/Map.h"
#include "Common/Array.h"
#include "Common/UUID.h"
#include "Assets/AssetManager.h"
#include "Assets/Blueprint.h"

static Array<Class>* GetListOfAllClasses() {
    static Array<Class>* s_ListOfAllClasses = nullptr;
    if (!s_ListOfAllClasses) {
        s_ListOfAllClasses = new Array<Class>();
        s_ListOfAllClasses->Add(Class("Object"));
    }
    return s_ListOfAllClasses;
}

static Map<String, String>* GetChildToParentClassnames() {
    static Map<String, String>* s_ChildToParentClassnames = nullptr;
    if (!s_ChildToParentClassnames) {
        s_ChildToParentClassnames = new Map<String, String>();
        // Reserve capacity for expected class count to avoid rehashing during initialization
        s_ChildToParentClassnames->Reserve(256);
    }
    return s_ChildToParentClassnames;
}

static Map<String, String>* GetClassnameToModuleNames() {
    static Map<String, String>* s_ClassnameToModuleNames = nullptr;
    if (!s_ClassnameToModuleNames) {
        s_ClassnameToModuleNames = new Map<String, String>();
        s_ClassnameToModuleNames->Reserve(256);
        (*s_ClassnameToModuleNames)["Object"] = "EngineCore";
    }
    return s_ClassnameToModuleNames;
}

const Class Class::None = Class("");

static Blueprint* FindBlueprint(const Class& InClass) {
    if (!InClass.IsBlueprint()) {
        return nullptr;
    }
    return AssetManager::Get().GetAsset<Blueprint>(InClass.GetBlueprintId());
}

Class Class::GetParentClass() const {
    if (IsBlueprint()) {
        Blueprint* blueprint = FindBlueprint(*this);
        return blueprint ? blueprint->GetRootClass() : Class::None;
    }
    if (!GetChildToParentClassnames()->ContainsKey(Name)) {
        return Class::None;
    }
    return GetChildToParentClassnames()->At(Name);
}

UUID Class::GetBlueprintId() const {
    return IsBlueprint() ? UUID::FromString(Name.substr(1)) : UUID::INVALID;
}

Class Class::FromBlueprint(const UUID& InBlueprintId) {
    return Class(String(1, BlueprintPrefix) + InBlueprintId.ToString());
}

String Class::GetDisplayName() const {
    if (Blueprint* blueprint = FindBlueprint(*this)) {
        return blueprint->GetDisplayName();
    }
    return Name;
}

String Class::GetModuleName() const {
    if (!GetClassnameToModuleNames()->ContainsKey(Name)) {
        return "";
    }
    return GetClassnameToModuleNames()->At(Name);
}

bool Class::IsSubclassOf(const Class& InBaseClass) const {
    if ((*this) == InBaseClass) return true;
    return (Name == "Object")
        ? false
        : (
            GetParentClass() == InBaseClass
                ? true
                : (
                    GetParentClass() != Class::None
                        ? GetParentClass().IsSubclassOf(InBaseClass)
                        : false
                )
        );
}

Array<Class> Class::GetSubclassesOf(const Class& InBaseClass) {
    Array<Class> subClasses;
    for (auto It : *GetListOfAllClasses()) {
        if (It != Class::None && It.IsSubclassOf(InBaseClass)) {
            subClasses.Add(It);
        }
    }
    return subClasses;
}

Array<Class> Class::GetAllClasses() {
    return *GetListOfAllClasses();
}

Class::RegisterParentChildClassRelationship::RegisterParentChildClassRelationship(const String& InChild, const String& InParent, const String& InModule) {
    (*GetChildToParentClassnames())[InChild] = InParent;
    (*GetClassnameToModuleNames())[InChild] = InModule;
    (*GetListOfAllClasses()).Add(Class(InChild));
}

void Class::RegisterRuntimeClass(const String& InName, const Class& InParent, const String& InModule,
                                 std::function<Object*()> InAllocator) {
    (*GetChildToParentClassnames())[InName] = InParent.Name;
    (*GetClassnameToModuleNames())[InName] = InModule;
    if (!GetListOfAllClasses()->Contains(Class(InName))) {
        GetListOfAllClasses()->Add(Class(InName));
    }
    Object::s_ObjectAllocators[InName] = std::move(InAllocator);
}

bool Class::IsDefaultConstructable() const {
    if (IsBlueprint()) {
        return FindBlueprint(*this) != nullptr;
    }
    return Object::s_ObjectAllocators.contains(Name);
}

Object* Object::Create(const Class& type) {
    if (type.IsBlueprint()) {
        Blueprint* blueprint = FindBlueprint(type);
        return blueprint ? (Object*)blueprint->CreateInstance() : nullptr;
    }
    auto allocator = s_ObjectAllocators.find(type.Name);
    return allocator != s_ObjectAllocators.end() ? allocator->second() : nullptr;
}

void Object::Retain(Object* InObject) {
    InternalPtr::Retain(InObject);
}

void Object::Release(Object* InObject) {
    InternalPtr::Release(InObject);
}

Object::~Object() {
    InternalPtr::Nullify(this);
}

bool Object::IsA(const Class& type) const {
    return IsObjectOfClass(type);
}
