#pragma once
#include <vector>

template<typename T>
struct Array : public std::vector<T> {
public:
    using std::vector<T>::vector; // Inherit all constructors
};