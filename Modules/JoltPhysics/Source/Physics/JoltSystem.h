#pragma once

namespace JPH {
    class JobSystem;
    class TempAllocator;
}

namespace JoltSystem {
    void Acquire();
    void Release();

    JPH::TempAllocator& GetTempAllocator();
    JPH::JobSystem& GetJobSystem();
}
