#include "Common/Array.h"
#include "Common/String.h"
#include "Windows.h"

int ArtifactMain(const Array<String>& InArgs);

#if !defined(AE_PACKAGED)

int main(int argc, char** argv) {
    Array<String> args;
    for (int i = 0; i < argc; i++) {
        args.Add(String(argv[i]));
    }
    return ArtifactMain(args);
}

#else

int WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    Array<String> args;
    args.Add("Artifact.exe");

    // Parse the command line arguments
    char* context = nullptr;
    char* token = strtok_s(lpCmdLine, " ", &context);
    while (token != nullptr) {
        args.Add(String(token));
        token = strtok_s(nullptr, " ", &context);
    }

    return ArtifactMain(args);
}

#endif