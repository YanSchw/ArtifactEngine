#pragma once
#include "Common/Types.h"
#include "Common/ByteString.h"

/* Collection of utility functions for platform specific file I/O */
class FileIO {
public:
    static String ReadFileToString(const String& InFilePath);
    static SharedObjectPtr<ByteString> ReadFileToBytes(const String& InFilePath);
    static SharedObjectPtr<ByteString> ReadFileRegion(const String& InFilePath, uint64_t InOffset, uint64_t InSize);

    static bool WriteStringToFile(const String& InFilePath, const String& InData);
    static bool WriteBytesToFile(const String& InFilePath, const ByteString& InData);
    static bool WriteBytesToFile(const String& InFilePath, const void* InData, uint64_t InSize);

    static uint64_t GetFileSize(const String& InFilePath);
    static bool FileExists(const String& InFilePath);
    static Array<String> ListFilesInDirectory(const String& InDirectoryPath, bool InRecursive = false);
};