#pragma once
#include "Common/Types.h"
#include "Common/ByteString.h"

/* Collection of utility functions for platform specific file I/O */
class FileIO {
public:
    static String ReadFileToString(const String& InFilePath);
    static SharedObjectPtr<ByteString> ReadFileToBytes(const String& InFilePath);

    static bool WriteStringToFile(const String& InFilePath, const String& InData);
    static bool WriteBytesToFile(const String& InFilePath, const ByteString& InData);
};