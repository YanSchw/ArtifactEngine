#include "Core/Log.h"

#pragma warning(push, 0)
#include <spdlog/spdlog.h>
#pragma warning(pop)

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <iostream>
#include <sstream>
#include <vector>
#include <filesystem>
#include <unordered_map>

static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> s_Loggers;

static constexpr bool IsPackagedBuild() {
#if defined(AE_PACKAGED)
    return true;
#else
    return false;
#endif
}

static std::filesystem::path GetPackagedBuildLogFileDirectory() {
    std::filesystem::path logFilePath;
#if defined(AE_PLATFORM_WINDOWS)
    logFilePath = std::filesystem::path(std::getenv("LOCALAPPDATA")) / "ArtifactEngine" / "Logs";
#elif defined(AE_PLATFORM_MACOS)
    logFilePath = std::filesystem::path(std::getenv("HOME")) / "Library" / "Logs" / "ArtifactEngine";
#elif defined(AE_PLATFORM_LINUX)
    logFilePath = std::filesystem::path(std::getenv("HOME")) / ".local" / "share" / "ArtifactEngine" / "Logs";
#else
    logFilePath = std::filesystem::current_path();
#endif
    return logFilePath;
}

static std::vector<std::string> split(const std::string &s, char delim) {
    std::vector<std::string> result;
    std::stringstream ss(s);
    std::string item;

    while (getline(ss, item, delim)) {
        result.push_back(item);
    }

    return result;
}

static std::string ConvertSourceFilenameToLoggerLabel(std::string name) {
    name = split(name, '/').back();
    name = split(name, '\\').back();
    name = split(name, '.').front();
    return name;
}

static void CreateLoggerIfNotExists(const std::string& name) {
    if (s_Loggers.find(name) != s_Loggers.end()) {
        return;
    }

    std::vector<spdlog::sink_ptr> logSinks;
    
    if (!IsPackagedBuild() /* || IsFlagSet("--LogToConsole") */) {
        auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        console_sink->set_pattern("%^[%T] %n: %v%$");
        logSinks.emplace_back(console_sink);
    }

    std::filesystem::path loggingDirectory = std::filesystem::current_path();
    if (IsPackagedBuild()) {
        std::filesystem::path logFilePath = GetPackagedBuildLogFileDirectory();
        loggingDirectory = logFilePath;
        std::filesystem::create_directories(loggingDirectory);
    }
    auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>(loggingDirectory / "Artifact.log", false);
    file_sink->set_pattern("[%T] [%l] %n: %v");
    logSinks.emplace_back(file_sink);

    std::string logger_label = std::string("[") + ConvertSourceFilenameToLoggerLabel(name) + std::string("]");
    s_Loggers[name] = std::make_shared<spdlog::logger>(logger_label, begin(logSinks), end(logSinks));
    spdlog::register_logger(s_Loggers.at(name));
    s_Loggers.at(name)->set_level(spdlog::level::trace);
    s_Loggers.at(name)->flush_on(spdlog::level::trace);
}

static std::shared_ptr<spdlog::logger>& GetLogger(const std::string& _file_) {
    CreateLoggerIfNotExists(_file_);
    return s_Loggers.at(_file_);
}

void Logging::Log(LogLevel level, const std::string& file, const std::string& message) {
    GetLogger(file)->log(static_cast<spdlog::level::level_enum>(level), message);
}