#pragma once

#include <cstdint>
#include <string>
#include <format>

enum class LogLevel : uint32_t {
    TRACE = 0,
    DEBUG = 1,
    INFO = 2,
    WARN = 3,
    ERROR = 4,
    CRITICAL = 5,
    OFF = 6
};

class Logging {
public:
    static void Log(LogLevel level, const std::string& file, const std::string& message);
};

// Main Logging Macro
#define AE_LOG(_LogLevel, ...) ::Logging::Log(_LogLevel, __FILE__, std::format(__VA_ARGS__))
#define AE_TRACE(...)          AE_LOG(::LogLevel::TRACE, __VA_ARGS__)
#define AE_INFO(...)           AE_LOG(::LogLevel::INFO, __VA_ARGS__)
#define AE_WARN(...)           AE_LOG(::LogLevel::WARN, __VA_ARGS__)
#define AE_ERROR(...)          AE_LOG(::LogLevel::ERROR, __VA_ARGS__)
#define AE_CRITICAL(...)       AE_LOG(::LogLevel::CRITICAL, __VA_ARGS__)