#pragma once
#include "Common/Types.h"

struct SubprocessResult {
    int ExitCode = -1;
    String StdOut;
    String StdErr;
};

class Subprocess {
public:
    static SubprocessResult Run(const String& InCommand);
};
