#include "EditorEngine.h"
#include "Core/Log.h"

void EditorEngine::Initialize() {

}
bool EditorEngine::MainTick(double InDeltaTime) {
    AE_INFO("FPS: {0}", 1.0f / InDeltaTime);
    return true;
}
void EditorEngine::Shutdown() {
    
}