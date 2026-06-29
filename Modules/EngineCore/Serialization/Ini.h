#pragma once
#include "CoreMinimal.h"

class IniParser {
public:
    static Map<String, Map<String, String>> Read(const String& InIni);
    static String Write(Map<String, Map<String, String>>& InMap);
};