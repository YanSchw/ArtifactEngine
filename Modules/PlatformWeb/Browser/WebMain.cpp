#include "Common/Array.h"
#include "Common/String.h"

int ArtifactMain(const Array<String>& InArgs);

int main(int argc, char** argv) {
    Array<String> args;
    for (int i = 0; i < argc; i++) {
        args.Add(String(argv[i]));
    }
    return ArtifactMain(args);
}
