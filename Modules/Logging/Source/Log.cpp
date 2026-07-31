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
#include <mutex>

#undef ERROR // windows.h/wingdi.h defines ERROR as 0, which clashes with LogLevel::ERROR

static std::unordered_map<std::string, std::shared_ptr<spdlog::logger>> s_Loggers;

static std::mutex s_CaptureMutex;
static std::vector<LogEntry> s_RecentEntries;
static uint64_t s_LogVersion = 0;
static int s_MessageCount = 0;
static int s_WarningCount = 0;
static int s_ErrorCount = 0;
static constexpr size_t s_MaxRecentEntries = 4000;

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
    std::error_code cwdError;
    logFilePath = std::filesystem::current_path(cwdError);
    if (cwdError) {
        logFilePath = ".";
    }
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

static spdlog::sink_ptr TryCreateFileSink(const std::filesystem::path& InDirectory) {
    try {
        std::error_code directoryError;
        std::filesystem::create_directories(InDirectory, directoryError);

        auto sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>((InDirectory / "Artifact.log").string(), true);
        sink->set_pattern("[%T] [%l] %n: %v");
        return sink;
    } catch (const spdlog::spdlog_ex& error) {
        std::cerr << "Could not open a log file in " << InDirectory.string() << ": " << error.what() << std::endl;
        return nullptr;
    }
}

static spdlog::sink_ptr GetFileSink() {
    static const spdlog::sink_ptr sink = [] {
        std::filesystem::path directory;
        if (IsPackagedBuild()) {
            directory = GetPackagedBuildLogFileDirectory();
        } else {
            std::error_code cwdError;
            directory = std::filesystem::current_path(cwdError);
            if (cwdError) {
                directory = GetPackagedBuildLogFileDirectory();
            }
        }

        if (spdlog::sink_ptr fileSink = TryCreateFileSink(directory)) {
            return fileSink;
        }

        const std::filesystem::path fallback = GetPackagedBuildLogFileDirectory();
        return fallback == directory ? spdlog::sink_ptr() : TryCreateFileSink(fallback);
    }();
    return sink;
}

static spdlog::sink_ptr GetConsoleSink() {
    static const spdlog::sink_ptr sink = [] {
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
        consoleSink->set_pattern("%^[%T] %n: %v%$");
        return consoleSink;
    }();
    return sink;
}

static void CreateLoggerIfNotExists(const std::string& name) {
    if (s_Loggers.find(name) != s_Loggers.end()) {
        return;
    }

    std::vector<spdlog::sink_ptr> logSinks;

    if (!IsPackagedBuild() /* || IsFlagSet("--LogToConsole") */) {
        logSinks.emplace_back(GetConsoleSink());
    }
    if (spdlog::sink_ptr fileSink = GetFileSink()) {
        logSinks.emplace_back(fileSink);
    }

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

    std::lock_guard<std::mutex> lock(s_CaptureMutex);
    s_RecentEntries.push_back(LogEntry{ level, ConvertSourceFilenameToLoggerLabel(file), message });
    if (s_RecentEntries.size() > s_MaxRecentEntries) {
        s_RecentEntries.erase(s_RecentEntries.begin(), s_RecentEntries.begin() + (s_MaxRecentEntries / 4));
    }
    if (level == LogLevel::WARN) {
        s_WarningCount++;
    } else if (level == LogLevel::ERROR || level == LogLevel::CRITICAL) {
        s_ErrorCount++;
    } else {
        s_MessageCount++;
    }
    s_LogVersion++;
}

std::vector<LogEntry> Logging::GetRecentEntries() {
    std::lock_guard<std::mutex> lock(s_CaptureMutex);
    return s_RecentEntries;
}

uint64_t Logging::GetLogVersion() {
    std::lock_guard<std::mutex> lock(s_CaptureMutex);
    return s_LogVersion;
}

int Logging::GetMessageCount() {
    std::lock_guard<std::mutex> lock(s_CaptureMutex);
    return s_MessageCount;
}

int Logging::GetWarningCount() {
    std::lock_guard<std::mutex> lock(s_CaptureMutex);
    return s_WarningCount;
}

int Logging::GetErrorCount() {
    std::lock_guard<std::mutex> lock(s_CaptureMutex);
    return s_ErrorCount;
}

void Logging::Clear() {
    std::lock_guard<std::mutex> lock(s_CaptureMutex);
    s_RecentEntries.clear();
    s_MessageCount = 0;
    s_WarningCount = 0;
    s_ErrorCount = 0;
    s_LogVersion++;
}