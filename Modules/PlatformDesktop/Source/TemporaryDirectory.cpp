#include "Core/Platform.h"
#include <filesystem>
#include <random>

static std::string GenerateRandomString(size_t length = 16) {
    static const char charset[] =
        "abcdefghijklmnopqrstuvwxyz"
        "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
        "0123456789";

    thread_local std::mt19937 rng{ std::random_device{}() };
    std::uniform_int_distribution<> dist(0, sizeof(charset) - 2);

    std::string result;
    result.reserve(length);

    for (size_t i = 0; i < length; ++i) {
        result += charset[dist(rng)];
    }

    return result;
}

Platform::TemporaryDirectory::TemporaryDirectory() {
    std::filesystem::path base = std::filesystem::temp_directory_path();

    while (true) {
        std::filesystem::path candidate = base / ("tmp_" + GenerateRandomString());

        if (std::filesystem::create_directory(candidate)) {
            Path = candidate.string();
            return;
        }
    }
}

Platform::TemporaryDirectory::~TemporaryDirectory() {
    if (!Path.empty()) {
        std::error_code ec;
        std::filesystem::remove_all(Path, ec);
    }
}