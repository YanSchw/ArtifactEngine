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