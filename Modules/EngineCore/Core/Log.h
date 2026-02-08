#pragma once

#include <string>
#include <format>

enum class LogLevel : int {
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
#define RT_LOG(_LogLevel, ...) ::Logging::Log(_LogLevel, __FILE__, std::format(__VA_ARGS__))
#define RT_TRACE(...)          RT_LOG(::LogLevel::TRACE, __VA_ARGS__)
#define RT_INFO(...)           RT_LOG(::LogLevel::INFO, __VA_ARGS__)
#define RT_WARN(...)           RT_LOG(::LogLevel::WARN, __VA_ARGS__)
#define RT_ERROR(...)          RT_LOG(::LogLevel::ERROR, __VA_ARGS__)
#define RT_CRITICAL(...)       RT_LOG(::LogLevel::CRITICAL, __VA_ARGS__)