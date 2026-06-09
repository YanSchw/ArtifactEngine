#pragma once
#include "CoreMinimal.h"

class ChunkReader;
class ChunkWriter;
class ChunkedBinary;
class ByteString;

class BinarySerializer {
public:
    static SharedObjectPtr<ByteString> SerializeObject(const Object* InObject);
    static void DeserializeObject(Object* OutObject, const SharedObjectPtr<ByteString>& InBinaryData);

    static SharedObjectPtr<ByteString> SerializeStruct(const void* InStruct, const Struct& InStructType);
    static void DeserializeStruct(void* OutStruct, const Struct& InStructType, const SharedObjectPtr<ByteString>& InBinaryData);

private:
    static void SerializeType(ChunkWriter& writer, void* instance, const String& typeName);
    static void DeserializeType(ChunkReader& reader, void* instance, const String& typeName);

    static void SerializeProperty(ChunkWriter& writer, Property* property, void* valuePtr);
    static void DeserializeProperty(ChunkReader& reader, Property* property, void* valuePtr);
};