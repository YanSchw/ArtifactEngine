#include "Platform/FileIO.h"

#include <cstring>
#include <filesystem>
#include <fstream>

// Emscripten mounts the packaged Content directory into its in-memory filesystem, so the standard
// library reaches it exactly like a native one.

String FileIO::ReadFileToString(const String& InFilePath) {
    std::ifstream file(InFilePath, std::ios::in | std::ios::binary);
    if (!file) {
        return "";
    }

    file.seekg(0, std::ios::end);
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (size <= 0) {
        return "";
    }

    String result;
    result.resize((size_t)size);
    file.read(&result[0], size);
    return result;
}

SharedObjectPtr<ByteString> FileIO::ReadFileToBytes(const String& InFilePath) {
    std::ifstream file(InFilePath, std::ios::in | std::ios::binary);
    if (!file) {
        return nullptr;
    }

    file.seekg(0, std::ios::end);
    const std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    if (size <= 0) {
        return nullptr;
    }

    byte* buffer = new byte[size];
    file.read(reinterpret_cast<char*>(buffer), size);
    return new ByteString((size_t)size, buffer);
}

SharedObjectPtr<ByteString> FileIO::ReadFileRegion(const String& InFilePath, uint64_t InOffset, uint64_t InSize) {
    std::ifstream file(InFilePath, std::ios::in | std::ios::binary);
    if (!file) {
        return nullptr;
    }

    file.seekg((std::streamoff)InOffset, std::ios::beg);
    if (!file) {
        return nullptr;
    }

    byte* buffer = new byte[InSize];
    file.read(reinterpret_cast<char*>(buffer), (std::streamsize)InSize);

    const uint64_t bytesRead = (uint64_t)file.gcount();
    if (bytesRead == 0) {
        delete[] buffer;
        return nullptr;
    }

    if (bytesRead != InSize) {
        byte* truncated = new byte[bytesRead];
        memcpy(truncated, buffer, bytesRead);
        delete[] buffer;
        buffer = truncated;
    }

    return new ByteString(bytesRead, buffer);
}

bool FileIO::WriteStringToFile(const String& InFilePath, const String& InData) {
    return WriteBytesToFile(InFilePath, InData.data(), InData.size());
}

bool FileIO::WriteBytesToFile(const String& InFilePath, const ByteString& InData) {
    return WriteBytesToFile(InFilePath, InData.GetData(), InData.GetSizeInBytes());
}

bool FileIO::WriteBytesToFile(const String& InFilePath, const void* InData, uint64_t InSize) {
    std::ofstream file(InFilePath, std::ios::out | std::ios::binary);
    if (!file) {
        return false;
    }

    file.write(reinterpret_cast<const char*>(InData), (std::streamsize)InSize);
    return file.good();
}

uint64_t FileIO::GetFileSize(const String& InFilePath) {
    std::error_code error;
    const uint64_t size = std::filesystem::file_size(InFilePath, error);
    return error ? 0 : size;
}

bool FileIO::FileExists(const String& InFilePath) {
    std::error_code error;
    return std::filesystem::exists(InFilePath, error) && !error;
}

Array<String> FileIO::ListFilesInDirectory(const String& InDirectoryPath, bool InRecursive) {
    Array<String> files;

    std::error_code error;
    if (!std::filesystem::is_directory(InDirectoryPath, error) || error) {
        return files;
    }

    const auto collect = [&files](const std::filesystem::directory_entry& InEntry) {
        std::error_code entryError;
        if (InEntry.is_regular_file(entryError) && !entryError) {
            files.Add(InEntry.path().string());
        }
    };

    if (InRecursive) {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(InDirectoryPath, error)) {
            collect(entry);
        }
    } else {
        for (const auto& entry : std::filesystem::directory_iterator(InDirectoryPath, error)) {
            collect(entry);
        }
    }

    return files;
}
