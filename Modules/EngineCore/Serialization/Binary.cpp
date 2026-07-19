#include "Binary.h"
#include "ChunkedBinary.h"

#include "Assets/AssetManager.h"
#include "Assets/Asset.h"
#include "Core/Assert.h"
#include "Object/Enum.h"

#include <cstring>

SharedObjectPtr<ByteString> BinarySerializer::SerializeObject(const Object* InObject) {
    ChunkWriter writer;

    String typeName = InObject->GetClass().Name;
    writer << typeName;  // Write type information first
    SerializeType(writer, (void*)InObject, typeName);

    return writer.GetData();
}

void BinarySerializer::DeserializeObject(Object* OutObject, const SharedObjectPtr<ByteString>& InBinaryData) {
    ChunkReader reader(InBinaryData);

    String typeName;
    reader >> typeName;

    AE_ASSERT(typeName == OutObject->GetClass().Name, "Binary type does not match object type");

    DeserializeType(reader, OutObject, OutObject->GetClass().Name);
}

SharedObjectPtr<ByteString> BinarySerializer::SerializeStruct(const void* InStruct, const Struct& InStructType) {
    ChunkWriter writer;
    SerializeType(writer, (void*)InStruct, InStructType.Name);
    return writer.GetData();
}

void BinarySerializer::DeserializeStruct(void* OutStruct, const Struct& InStructType, const SharedObjectPtr<ByteString>& InBinaryData) {
    ChunkReader reader(InBinaryData);
    DeserializeType(reader, OutStruct, InStructType.Name);
}

void BinarySerializer::SerializeType(ChunkWriter& writer, void* instance, const String& typeName) {
    auto props = Property::GetTypeProperties(typeName);

    for (Property* prop : props) {
        void* valuePtr = (char*)instance + prop->Offset;
        SerializeProperty(writer, prop, valuePtr);
    }
}

void BinarySerializer::DeserializeType(ChunkReader& reader, void* instance, const String& typeName) {
    auto props = Property::GetTypeProperties(typeName);

    for (Property* prop : props) {
        void* valuePtr = (char*)instance + prop->Offset;
        DeserializeProperty(reader, prop, valuePtr);
    }
}

void BinarySerializer::SerializeProperty(ChunkWriter& writer, Property* property, void* valuePtr) {
    if (auto p = Cast<IntProperty>(property)) {
        if (p->IsUnsigned) {
            switch (p->NumBits) {
                case 8:  writer << *(uint8_t*)valuePtr; break;
                case 16: writer << *(uint16_t*)valuePtr; break;
                case 32: writer << *(uint32_t*)valuePtr; break;
                case 64: writer << *(uint64_t*)valuePtr; break;
            }
        } else {
            switch (p->NumBits) {
                case 8:  writer << *(int8_t*)valuePtr; break;
                case 16: writer << *(int16_t*)valuePtr; break;
                case 32: writer << *(int32_t*)valuePtr; break;
                case 64: writer << *(int64_t*)valuePtr; break;
            }
        }
    } else if (auto p = Cast<FloatProperty>(property)) {
        if (p->IsDouble) {
            writer << *(double*)valuePtr;
        } else {
            writer << *(float*)valuePtr;
        }
    } else if (auto p = Cast<BoolProperty>(property)) {
        writer << *(bool*)valuePtr;
    } else if (auto p = Cast<StringProperty>(property)) {
        writer << *(String*)valuePtr;
    } else if (auto p = Cast<SharedObjectPtrProperty>(property)) {
        auto& ptr = *(SharedObjectPtr<Object>*)valuePtr;

        if (!ptr) {
            // Write null marker
            writer << (uint8_t)0;
        } else {
            // Write non-null marker
            writer << (uint8_t)1;

            auto serialized = SerializeObject(ptr.Get());
            writer << (uint64_t)serialized->GetSizeInBytes();
            writer.WriteBytes(serialized->GetData(), serialized->GetSizeInBytes());
        }
    } else if (auto p = Cast<WeakObjectPtrProperty>(property)) {
        // Weak references don't own their target; only asset targets round-trip, by id.
        Asset* asset = Cast<Asset>((*(WeakObjectPtr<Object>*)valuePtr).Get());
        writer << (asset ? asset->GetId().ToString() : String(""));
    } else if (auto p = Cast<ArrayProperty>(property)) {
        size_t count = p->GetSize(valuePtr);
        writer << (uint64_t)count;

        for (size_t i = 0; i < count; ++i) {
            void* elem = p->GetElementPtr(valuePtr, i);
            SerializeProperty(writer, p->InnerProperty, elem);
        }
    } else if (auto p = Cast<EnumProperty>(property)) {
        int64_t value = 0;
        std::memcpy(&value, valuePtr, p->ByteSize);
        writer << Enum(p->InnerEnumTypename).ConvertValueToString(value);
    } else {
        AE_ASSERT(false, "Unsupported property type for serialization: " + property->Name);
    }
}

void BinarySerializer::DeserializeProperty(ChunkReader& reader, Property* property, void* valuePtr) {
    if (auto p = Cast<IntProperty>(property)) {
        if (p->IsUnsigned) {
            switch (p->NumBits) {
                case 8:  { uint8_t v; reader >> v; *(uint8_t*)valuePtr = v; } break;
                case 16: { uint16_t v; reader >> v; *(uint16_t*)valuePtr = v; } break;
                case 32: { uint32_t v; reader >> v; *(uint32_t*)valuePtr = v; } break;
                case 64: { uint64_t v; reader >> v; *(uint64_t*)valuePtr = v; } break;
            }
        } else {
            switch (p->NumBits) {
                case 8:  { int8_t v; reader >> v; *(int8_t*)valuePtr = v; } break;
                case 16: { int16_t v; reader >> v; *(int16_t*)valuePtr = v; } break;
                case 32: { int32_t v; reader >> v; *(int32_t*)valuePtr = v; } break;
                case 64: { int64_t v; reader >> v; *(int64_t*)valuePtr = v; } break;
            }
        }
    } else if (auto p = Cast<FloatProperty>(property)) {
        if (p->IsDouble) {
            double v;
            reader >> v;
            *(double*)valuePtr = v;
        } else {
            float v;
            reader >> v;
            *(float*)valuePtr = v;
        }
    } else if (auto p = Cast<BoolProperty>(property)) {
        bool v;
        reader >> v;
        *(bool*)valuePtr = v;
    } else if (auto p = Cast<StringProperty>(property)) {
        String v;
        reader >> v;
        *(String*)valuePtr = v;
    } else if (auto p = Cast<SharedObjectPtrProperty>(property)) {
        uint8_t isNull;
        reader >> isNull;

        if (isNull == 0) {
            *(SharedObjectPtr<Object>*)valuePtr = nullptr;
        } else {
            uint64_t dataSize;
            reader >> dataSize;

            byte* buffer = new byte[dataSize];
            reader.ReadBytes(buffer, dataSize);
            auto data = SharedObjectPtr<ByteString>(new ByteString(dataSize, buffer));

            // Peek the concrete type recorded by SerializeObject so polymorphic
            // pointers create the right subclass, not the declared inner class.
            ChunkReader peeker(data);
            String concreteType;
            peeker >> concreteType;

            Object* obj = Object::Create(Class(concreteType));
            DeserializeObject(obj, data);
            *(SharedObjectPtr<Object>*)valuePtr = SharedObjectPtr<Object>(obj);
        }
    } else if (auto p = Cast<WeakObjectPtrProperty>(property)) {
        String id;
        reader >> id;
        *(WeakObjectPtr<Object>*)valuePtr = id.empty()
            ? nullptr
            : (Object*)AssetManager::Get().GetAsset(UUID::FromString(id));
    } else if (auto p = Cast<ArrayProperty>(property)) {
        uint64_t count;
        reader >> count;

        for (uint64_t i = 0; i < count; ++i) {
            p->AddDefault(valuePtr);

            size_t last = p->GetSize(valuePtr) - 1;
            void* elemPtr = p->GetElementPtr(valuePtr, last);

            DeserializeProperty(reader, p->InnerProperty, elemPtr);
        }
    } else if (auto p = Cast<EnumProperty>(property)) {
        String name;
        reader >> name;
        int64_t value = Enum(p->InnerEnumTypename).ConvertStringToValue(name);
        std::memcpy(valuePtr, &value, p->ByteSize);
    } else {
        AE_ASSERT(false, "Unsupported property type for deserialization: " + property->Name);
    }
}