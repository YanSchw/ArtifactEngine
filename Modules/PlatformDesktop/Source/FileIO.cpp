#include "Platform/FileIO.h"
#include <fstream>
#include <vector>

String FileIO::ReadFileToString(const String& InFilePath) {
    std::ifstream file(InFilePath, std::ios::in | std::ios::binary);
    if (!file)
        return "";

    // Read entire file into string
    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size <= 0)
        return "";

    String result;
    result.resize(static_cast<size_t>(size));

    file.read(&result[0], size);
    return result;
}

SharedObjectPtr<ByteString> FileIO::ReadFileToBytes(const String& InFilePath) {
    std::ifstream file(InFilePath, std::ios::in | std::ios::binary);
    if (!file)
        return nullptr;

    file.seekg(0, std::ios::end);
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);

    if (size <= 0)
        return nullptr;

    byte* buffer = new byte[size];
    file.read(reinterpret_cast<char*>(buffer), size);

    return new ByteString(static_cast<size_t>(size), buffer);
}

bool FileIO::WriteStringToFile(const String& InFilePath, const String& InData) {
    std::ofstream file(InFilePath, std::ios::out | std::ios::binary);
    if (!file)
        return false;

    file.write(InData.data(), static_cast<std::streamsize>(InData.size()));
    return file.good();
}

bool FileIO::WriteBytesToFile(const String& InFilePath, const ByteString& InData) {
    std::ofstream file(InFilePath, std::ios::out | std::ios::binary);
    if (!file)
        return false;

    file.write(reinterpret_cast<const char*>(InData.GetData()),
               static_cast<std::streamsize>(InData.GetSizeInBytes()));

    return file.good();
}