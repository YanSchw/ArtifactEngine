#include <iostream>

#include "Log.h"

int ArtifactMain() {
    RT_INFO("Hello, Artifact Engine! This is a test log message.");
    return 0;
}

int main() {
    return ArtifactMain();
}