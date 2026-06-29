#include "Ini.h"

namespace {
    String Trim(const String& InString) {
        size_t first = InString.find_first_not_of(" \t\r\n");
        if (first == String::npos) {
            return "";
        }
        size_t last = InString.find_last_not_of(" \t\r\n");
        return InString.substr(first, last - first + 1);
    }
}

Map<String, Map<String, String>> IniParser::Read(const String& InIni) {
    Map<String, Map<String, String>> result;

    // The global section (keys before any [Section] header) uses the empty string.
    String currentSection = "";
    result[currentSection];

    size_t lineStart = 0;
    while (lineStart <= InIni.size()) {
        size_t lineEnd = InIni.find('\n', lineStart);
        if (lineEnd == String::npos) {
            lineEnd = InIni.size();
        }

        String line = Trim(InIni.substr(lineStart, lineEnd - lineStart));
        lineStart = lineEnd + 1;

        if (line.empty() || line[0] == ';' || line[0] == '#') {
            continue;
        }

        if (line[0] == '[' && line.back() == ']') {
            currentSection = Trim(line.substr(1, line.size() - 2));
            result[currentSection];
            continue;
        }

        size_t equals = line.find('=');
        if (equals == String::npos) {
            continue;
        }

        String key = Trim(line.substr(0, equals));
        String value = Trim(line.substr(equals + 1));
        result[currentSection][key] = value;
    }

    return result;
}

String IniParser::Write(Map<String, Map<String, String>>& InMap) {
    String result;

    // Emit the global section first (keys with no [Section] header).
    if (InMap.ContainsKey("")) {
        for (const auto& [key, value] : InMap.At("")) {
            result += key + "=" + value + "\n";
        }
    }

    for (const auto& [section, entries] : InMap) {
        if (section.empty()) {
            continue;
        }

        if (!result.empty()) {
            result += "\n";
        }
        result += "[" + section + "]\n";
        for (const auto& [key, value] : entries) {
            result += key + "=" + value + "\n";
        }
    }

    return result;
}
