#pragma once
#include <functional>
#include <string>
#include <unordered_map>

struct Struct {
    std::string Name;
    
    Struct() : Name("") {};
    Struct(const Struct&) = default;
    Struct(const std::string& name) : Name(name) {}
};


#ifndef GENERATED_BODY /* in-case Object.h is included */
#define GENERATED_BODY_CONCAT(line) _GENERATED_BODY_##line
#define GENERATED_BODY(line) GENERATED_BODY_CONCAT(line)
#endif

#define ARTIFACT_STRUCT(...) GENERATED_BODY(__LINE__)
