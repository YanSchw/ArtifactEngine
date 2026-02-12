#include <iostream>

#include "Log.h"
#include "Engine.h"

int ArtifactMain() {
    AE_INFO("Hello, Artifact Engine! This is a test log message.");
    return 0;
}

int main() {
    return ArtifactMain();
}