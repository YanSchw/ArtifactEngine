#include "MainThread.h"
#include <thread>

static std::thread::id s_MainThread;
static thread_local bool s_InLocalUpdate = false;

void MainThread::Register() {
    s_MainThread = std::this_thread::get_id();
}

bool MainThread::IsCurrent() {
    if (s_InLocalUpdate) {
        return false;
    }

    return s_MainThread == std::thread::id() || s_MainThread == std::this_thread::get_id();
}

MainThread::LocalUpdateScope::LocalUpdateScope() {
    s_InLocalUpdate = true;
}

MainThread::LocalUpdateScope::~LocalUpdateScope() {
    s_InLocalUpdate = false;
}
