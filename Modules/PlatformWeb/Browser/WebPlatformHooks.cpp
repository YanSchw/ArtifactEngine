#include "WebPlatformHooks.h"

#include <emscripten/emscripten.h>

static std::function<bool()> s_Tick;

extern "C" EMSCRIPTEN_KEEPALIVE int ArtifactTickFrame() {
    return s_Tick && s_Tick() ? 1 : 0;
}

void WebPlatformHooks::RunMainLoop(const std::function<bool()>& InTick) {
    s_Tick = InTick;

    EM_ASM({
        const step = () => {
            if (Module._ArtifactTickFrame()) {
                requestAnimationFrame(step);
            }
        };
        requestAnimationFrame(step);
    });

    // Leaves main() without tearing the process down, so the frames scheduled above keep running.
    emscripten_exit_with_live_runtime();
}
