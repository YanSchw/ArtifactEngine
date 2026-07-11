#include "Platform/FileIO.h"
#include <fstream>
#include <vector>
#include <cstring>

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

bool FileIO::WriteBytesToFile(const String& InFilePath, const void* InData, uint64_t InSize) {
    std::ofstream file(InFilePath, std::ios::out | std::ios::binary);
    if (!file)
        return false;

    file.write(reinterpret_cast<const char*>(InData),
               static_cast<std::streamsize>(InSize));

    return file.good();
}

SharedObjectPtr<ByteString> FileIO::ReadFileRegion(
    const String& InFilePath,
    uint64_t InOffset,
    uint64_t InSize) {
    std::ifstream file(InFilePath, std::ios::in | std::ios::binary);
    if (!file)
        return nullptr;

    file.seekg(static_cast<std::streamoff>(InOffset), std::ios::beg);

    if (!file)
        return nullptr;

    byte* buffer = new byte[InSize];
    file.read(reinterpret_cast<char*>(buffer), static_cast<std::streamsize>(InSize));

    if (!file && file.gcount() == 0) {
        delete[] buffer;
        return nullptr;
    }

    // Resize buffer to actual bytes read in case of EOF
    uint64_t bytesRead = static_cast<uint64_t>(file.gcount());

    if (bytesRead != InSize) {
        byte* resizedBuffer = new byte[bytesRead];
        memcpy(resizedBuffer, buffer, bytesRead);
        delete[] buffer;
        buffer = resizedBuffer;
    }

    return new ByteString(bytesRead, buffer);
}

uint64_t FileIO::GetFileSize(const String& InFilePath) {
    namespace fs = std::filesystem;

    std::error_code ec;

    uint64_t size = fs::file_size(InFilePath, ec);

    if (ec)
        return 0;

    return size;
}

bool FileIO::FileExists(const String& InFilePath) {
    namespace fs = std::filesystem;

    std::error_code ec;

    return fs::exists(InFilePath, ec) && !ec;
}

Array<String> FileIO::ListFilesInDirectory(const String& InDirectoryPath, bool InRecursive) {
    Array<String> files;

    namespace fs = std::filesystem;

    std::error_code ec;

    if (!fs::exists(InDirectoryPath, ec) || !fs::is_directory(InDirectoryPath, ec)) {
        return files;
    }

    if (InRecursive) {
        fs::recursive_directory_iterator it(
            InDirectoryPath,
            fs::directory_options::skip_permission_denied,
            ec);

        fs::recursive_directory_iterator end;

        while (!ec && it != end) {
            std::error_code entryEc;

            if (it->is_regular_file(entryEc) && !entryEc) {
                files.Add(it->path().string());
            }

            it.increment(ec);
        }
    } else {
        fs::directory_iterator it(
            InDirectoryPath,
            fs::directory_options::skip_permission_denied,
            ec);

        fs::directory_iterator end;

        while (!ec && it != end) {
            std::error_code entryEc;

            if (it->is_regular_file(entryEc) && !entryEc) {
                files.Add(it->path().string());
            }

            it.increment(ec);
        }
    }

    return files;
}