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
    logSinks.emplace_back(std::make_shared<spdlog::sinks::stdout_color_sink_mt>());
    logSinks.emplace_back(std::make_shared<spdlog::sinks::basic_file_sink_mt>("Artifact.log", false));

    logSinks[0]->set_pattern("%^[%T] %n: %v%$");
    logSinks[1]->set_pattern("[%T] [%l] %n: %v");

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