#pragma once
#include <string>

// STL wrapper around std::string to allow extensions.
struct String : public std::string {
public:
    using std::string::string; // Inherit all constructors
};