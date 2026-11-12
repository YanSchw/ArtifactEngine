#include "ScriptCompiler.h"
#include <cstdio>

#if defined(_WIN32)
    #define AE_POPEN _popen
    #define AE_PCLOSE _pclose
#else
    #define AE_POPEN popen
    #define AE_PCLOSE pclose
#endif

ScriptCompiler::~ScriptCompiler() {
    if (m_Thread.joinable()) {
        m_Thread.join();
    }
}

void ScriptCompiler::Start(const String& InCommand) {
    if (m_Thread.joinable()) {
        m_Thread.join();
    }
    m_Finished = false;
    m_Thread = std::thread([this, InCommand] {
        String output;
        int status = -1;
#if defined(_WIN32)
        const String command = "\"" + InCommand + " 2>&1\"";
#else
        const String command = InCommand + " 2>&1";
#endif
        if (FILE* pipe = AE_POPEN(command.c_str(), "r")) {
            char buffer[512];
            while (std::fgets(buffer, sizeof(buffer), pipe)) {
                output += buffer;
            }
            status = AE_PCLOSE(pipe);
        }
        m_Output = std::move(output);
        m_Succeeded = status == 0;
        m_Finished = true;
    });
}

bool ScriptCompiler::TakeResult(bool& OutSucceeded, String& OutOutput) {
    if (!m_Thread.joinable() || !m_Finished) {
        return false;
    }
    m_Thread.join();
    OutSucceeded = m_Succeeded;
    OutOutput = std::move(m_Output);
    return true;
}
