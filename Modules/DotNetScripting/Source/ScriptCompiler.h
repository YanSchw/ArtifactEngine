#pragma once
#include "Common/Types.h"
#include <atomic>
#include <thread>

class ScriptCompiler {
public:
    ~ScriptCompiler();

    void Start(const String& InCommand);
    bool IsRunning() const { return m_Thread.joinable() && !m_Finished; }
    bool TakeResult(bool& OutSucceeded, String& OutOutput);

private:
    std::thread m_Thread;
    std::atomic<bool> m_Finished = false;
    bool m_Succeeded = false;
    String m_Output;
};
