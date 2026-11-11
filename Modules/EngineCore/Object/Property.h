#pragma once
#include "Object.h"
#include "Common/String.h"
#include "Common/Map.h"
#include "Common/Array.h"
#include "SubclassOf.h"
#include <functional>
#include <memory>
#include <type_traits>
#include "Property.gen.h"

template<typename... Args> class Delegate;
struct DelegateHandle;
struct Function;

#define PROPERTY(...)

struct Property : public Object {
    ARTIFACT_CLASS();

    using ChangedFn = void(*)(void*);

    String Name;
    uint64_t Offset;
    ChangedFn OnChanged = nullptr;
    String Comment;
    bool HasRange = false;
    double RangeMin = 0.0;
    double RangeMax = 0.0;

    Property(const std::string& name, uint64_t offset)
        : Name(name), Offset(offset) {}

    void* GetValuePtr(void* InInstance) const { return (char*)InInstance + Offset; }
    void NotifyChanged(void* InInstance) const { if (OnChanged) OnChanged(InInstance); }

    Property* Changed(ChangedFn InChanged) { OnChanged = InChanged; return this; }
    Property* SetComment(const String& InComment) { Comment = InComment; return this; }
    Property* SetRange(double InMin, double InMax) { HasRange = true; RangeMin = InMin; RangeMax = InMax; return this; }
    double ClampToRange(double InValue) const {
        return !HasRange ? InValue : InValue < RangeMin ? RangeMin : InValue > RangeMax ? RangeMax : InValue;
    }
    /** Returns true if the property is transient and should not be saved. */
    bool IsTransient() const;

    void CopyValue(void* OutInstance, const void* InInstance) const;
    void CopyRawValue(void* OutValue, const void* InValue) const;

    static void RegisterTypeProperties(const String& InTypename, const Array<Property*>& InProperties);
    static Array<Property*> GetTypeProperties(const String& InTypeName);
    static Array<Property*> GetAllTypeProperties(const Class& InClass);
    static Property* FindTypeProperty(const Class& InClass, const String& InName);
};

struct IntProperty : public Property {
    ARTIFACT_CLASS();

    bool IsUnsigned;
    uint8_t NumBits;

    IntProperty(const std::string& name, uint64_t offset, bool isUnsigned, uint8_t numBits)
        : Property(name, offset), IsUnsigned(isUnsigned), NumBits(numBits) {}
};

struct FloatProperty : public Property {
    ARTIFACT_CLASS();

    bool IsDouble;

    FloatProperty(const std::string& name, uint64_t offset, bool isDouble)
        : Property(name, offset), IsDouble(isDouble) {}
};

struct BoolProperty : public Property {
    ARTIFACT_CLASS();

    BoolProperty(const std::string& name, uint64_t offset)
        : Property(name, offset) {}
};

struct StringProperty : public Property {
    ARTIFACT_CLASS();

    StringProperty(const std::string& name, uint64_t offset)
        : Property(name, offset) {}
};

struct SharedObjectPtrProperty : public Property {
    ARTIFACT_CLASS();

    Class InnerClass;

    SharedObjectPtrProperty(const std::string& name, uint64_t offset, const Class& innerClass)
        : Property(name, offset), InnerClass(innerClass) {}
};

struct WeakObjectPtrProperty : public Property {
    ARTIFACT_CLASS();

    Class InnerClass;

    WeakObjectPtrProperty(const std::string& name, uint64_t offset, const Class& innerClass)
        : Property(name, offset), InnerClass(innerClass) {}
};

struct UUIDProperty : public Property {
    ARTIFACT_CLASS();

    UUIDProperty(const std::string& name, uint64_t offset)
        : Property(name, offset) {}
};

struct StructProperty : public Property {
    ARTIFACT_CLASS();

    String InnerStructTypename;

    StructProperty(const std::string& name, uint64_t offset, const String& innerStructTypename)
        : Property(name, offset), InnerStructTypename(innerStructTypename) {}
};

struct EnumProperty : public Property {
    ARTIFACT_CLASS();

    String InnerEnumTypename;
    uint8_t ByteSize;  // size of the enum's underlying type, for raw memory access

    EnumProperty(const std::string& name, uint64_t offset, const String& innerEnumTypename, uint8_t byteSize)
        : Property(name, offset), InnerEnumTypename(innerEnumTypename), ByteSize(byteSize) {}
};

struct ArrayProperty : public Property {
    ARTIFACT_CLASS();

    using GetSizeFn = size_t(*)(void*);
    using GetElementPtrFn = void*(*)(void*, size_t);
    using AddDefaultFn = void(*)(void*);
    using ClearFn = void(*)(void*);
    using RemoveAtFn = void(*)(void*, size_t);
    using SwapFn = void(*)(void*, size_t, size_t);

    Property* InnerProperty;
    GetSizeFn GetSize;
    GetElementPtrFn GetElementPtr;
    AddDefaultFn AddDefault;
    ClearFn Clear;
    RemoveAtFn RemoveAt;
    SwapFn Swap;

    ArrayProperty(const std::string& name, uint64_t offset, Property* innerProperty, GetSizeFn getSize, GetElementPtrFn getElementPtr,
                  AddDefaultFn addDefault, ClearFn clear, RemoveAtFn removeAt, SwapFn swap)
        : Property(name, offset), InnerProperty(innerProperty), GetSize(getSize), GetElementPtr(getElementPtr), AddDefault(addDefault),
          Clear(clear), RemoveAt(removeAt), Swap(swap) {}
};

struct MapProperty : public Property {
    ARTIFACT_CLASS();

    struct Entry {
        const void* Key;
        void* Value;
    };

    using GetSizeFn = size_t(*)(void*);
    using GetEntriesFn = void(*)(void*, Array<Entry>&);
    using FindFn = void*(*)(void*, const void*);
    using FindOrAddFn = void*(*)(void*, const void*);
    using RemoveFn = void(*)(void*, const void*);
    using RenameKeyFn = bool(*)(void*, const void*, const void*);
    using ClearFn = void(*)(void*);
    using NewKeyFn = void*(*)();
    using DeleteKeyFn = void(*)(void*);

    Property* KeyProperty;
    Property* ValueProperty;
    GetSizeFn GetSize;
    GetEntriesFn GetEntries;
    FindFn Find;
    FindOrAddFn FindOrAdd;
    RemoveFn Remove;
    RenameKeyFn RenameKey;
    ClearFn Clear;
    NewKeyFn NewKey;
    DeleteKeyFn DeleteKey;

    MapProperty(const std::string& name, uint64_t offset, Property* keyProperty, Property* valueProperty, GetSizeFn getSize,
                GetEntriesFn getEntries, FindFn find, FindOrAddFn findOrAdd, RemoveFn remove, RenameKeyFn renameKey, ClearFn clear,
                NewKeyFn newKey, DeleteKeyFn deleteKey)
        : Property(name, offset), KeyProperty(keyProperty), ValueProperty(valueProperty), GetSize(getSize), GetEntries(getEntries),
          Find(find), FindOrAdd(findOrAdd), Remove(remove), RenameKey(renameKey), Clear(clear), NewKey(newKey), DeleteKey(deleteKey) {}

    std::shared_ptr<void> MakeKey() const { return std::shared_ptr<void>(NewKey(), DeleteKey); }
};

struct SubclassOfProperty : public Property {
    ARTIFACT_CLASS();

    Class BaseClass;

    SubclassOfProperty(const std::string& name, uint64_t offset, const Class& baseClass)
        : Property(name, offset), BaseClass(baseClass) {}

    bool Accepts(const Class& InClass) const;
    const Class& Read(const void* InValuePtr) const { return *(const Class*)InValuePtr; }
    bool Write(void* InValuePtr, const Class& InClass) const;
};

struct DelegateProperty : public Property {
    ARTIFACT_CLASS();

    using GetBindingCountFn = int32_t(*)(const void*);
    using BindFn = DelegateHandle(*)(void*, Object*, std::function<void(void*)>);
    using UnbindFn = void(*)(void*, DelegateHandle);

    String Signature;
    Function* SignatureFunction;
    GetBindingCountFn GetBindingCount;
    BindFn Bind;
    UnbindFn Unbind;

    DelegateProperty(const std::string& name, uint64_t offset, const String& signature, Function* signatureFunction,
                     GetBindingCountFn getBindingCount, BindFn bind, UnbindFn unbind)
        : Property(name, offset), Signature(signature), SignatureFunction(signatureFunction), GetBindingCount(getBindingCount),
          Bind(bind), Unbind(unbind) {}
};

/** A property reached through nested struct members, written as a dotted path such as
 *  "m_Params.Bloom.Intensity". A path of one segment is a plain property. */
struct PropertyPath {
    /** The first segment, the property that owns the change hooks. */
    Property* Root = nullptr;
    Property* Leaf = nullptr;
    /** Offset of the struct holding Leaf, from the start of the instance. */
    uint64_t ContainerOffset = 0;

    bool IsValid() const { return Leaf != nullptr; }
    void* GetValuePtr(void* InInstance) const { return (char*)InInstance + ContainerOffset + Leaf->Offset; }
    void CopyValue(void* OutInstance, const void* InInstance) const;
    void CopyRawValue(void* OutValue, const void* InValue) const;

    static PropertyPath Resolve(const Class& InClass, const String& InPath);
    static String GetRootName(const String& InPath);
    /** True when InPath is InAncestor itself or lies inside it. */
    static bool IsWithin(const String& InPath, const String& InAncestor);
};

namespace PropertyTypes {
    template<typename T> struct IsArray : std::false_type {};
    template<typename T> struct IsArray<Array<T>> : std::true_type {};
    template<typename T> struct IsMap : std::false_type {};
    template<typename K, typename V> struct IsMap<Map<K, V>> : std::true_type {};
    template<typename T> struct IsSubclassOf : std::false_type {};
    template<typename T> struct IsSubclassOf<SubclassOf<T>> : std::true_type {};
    template<typename T> struct IsDelegate : std::false_type {};
    template<typename... Args> struct IsDelegate<Delegate<Args...>> : std::true_type {};

    template<typename T>
    inline bool Matches(Property* InProperty) {
        if constexpr (std::is_same_v<T, bool>) {
            return Cast<BoolProperty>(InProperty) != nullptr;
        } else if constexpr (std::is_same_v<T, float> || std::is_same_v<T, double>) {
            FloatProperty* property = Cast<FloatProperty>(InProperty);
            return property && property->IsDouble == std::is_same_v<T, double>;
        } else if constexpr (std::is_enum_v<T>) {
            EnumProperty* property = Cast<EnumProperty>(InProperty);
            return property && property->ByteSize == sizeof(T);
        } else if constexpr (std::is_integral_v<T>) {
            IntProperty* property = Cast<IntProperty>(InProperty);
            return property && property->NumBits == sizeof(T) * 8 && property->IsUnsigned == std::is_unsigned_v<T>;
        } else if constexpr (std::is_same_v<T, String>) {
            return Cast<StringProperty>(InProperty) != nullptr;
        } else if constexpr (std::is_same_v<T, UUID>) {
            return Cast<UUIDProperty>(InProperty) != nullptr;
        } else if constexpr (IsArray<T>::value) {
            return Cast<ArrayProperty>(InProperty) != nullptr;
        } else if constexpr (IsMap<T>::value) {
            return Cast<MapProperty>(InProperty) != nullptr;
        } else if constexpr (IsSubclassOf<T>::value) {
            SubclassOfProperty* property = Cast<SubclassOfProperty>(InProperty);
            return property && property->BaseClass.IsSubclassOf(T::GetBaseClass());
        } else if constexpr (IsDelegate<T>::value) {
            return Cast<DelegateProperty>(InProperty) != nullptr;
        } else {
            return Cast<StructProperty>(InProperty) != nullptr;
        }
    }
}

template<typename T>
bool Object::GetPropertyValue(const String& InName, T& OutValue) {
    Property* property = Property::FindTypeProperty(GetClass(), InName);
    if (!property || !PropertyTypes::Matches<T>(property)) {
        return false;
    }
    OutValue = *(const T*)property->GetValuePtr(this);
    return true;
}
