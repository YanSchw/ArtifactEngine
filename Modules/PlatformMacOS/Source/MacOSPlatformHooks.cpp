#include "MacOSPlatformHooks.h"
#include "CoreMinimal.h"

#include <CoreFoundation/CoreFoundation.h>
#include <dlfcn.h>
#include <cstdlib>

// AppKit is reached through the raw Objective-C runtime: its headers typedef `Class`,
// which collides with the engine's reflection type of the same name.
extern "C" {
    void* objc_getClass(const char* InName);
    void* sel_registerName(const char* InName);
    void objc_msgSend(void);
    void* objc_allocateClassPair(void* InSuperclass, const char* InName, size_t InExtraBytes);
    void objc_registerClassPair(void* InClass);
    bool class_addMethod(void* InClass, void* InSelector, void (*InImp)(void), const char* InTypes);
}

static void* SendMessage(void* InReceiver, const char* InSelector) {
    return ((void* (*)(void*, void*))objc_msgSend)(InReceiver, sel_registerName(InSelector));
}

static void* SendMessage(void* InReceiver, const char* InSelector, void* InArg) {
    return ((void* (*)(void*, void*, void*))objc_msgSend)(InReceiver, sel_registerName(InSelector), InArg);
}

void MacOSPlatformHooks::SetApplicationIcon(const String& InImagePath) {
    void* path = ((void* (*)(void*, void*, const char*))objc_msgSend)(
        objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), InImagePath.c_str());
    void* image = SendMessage(SendMessage(objc_getClass("NSImage"), "alloc"), "initWithContentsOfFile:", path);
    if (!image) {
        AE_WARN("Failed to load application icon '{0}'", InImagePath);
        return;
    }

    void* app = SendMessage(objc_getClass("NSApplication"), "sharedApplication");
    SendMessage(app, "setApplicationIconImage:", image);
    SendMessage(image, "release");
}

static void* MakeString(const char* InText) {
    return ((void* (*)(void*, void*, const char*))objc_msgSend)(
        objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"), InText);
}

static bool s_FullscreenTransitionFinished = false;

static void OnFullscreenTransitionFinished(void*, void*, void*) {
    s_FullscreenTransitionFinished = true;
}

/* The transition into a full-screen space is animated, and anything presented to the window
 * before it finishes goes to a layer macOS is no longer showing, the switch is driven to
 * completion here, leaving callers with a window that is genuinely ready to be rendered to. */
static void WaitForFullscreenTransition(void* InNativeWindow, bool InFullscreen) {
    static void* observer = [] {
        void* observerClass = objc_allocateClassPair(objc_getClass("NSObject"), "ArtifactFullscreenObserver", 0);
        class_addMethod(observerClass, sel_registerName("onTransitionFinished:"),
                        (void (*)(void))OnFullscreenTransitionFinished, "v@:@");
        objc_registerClassPair(observerClass);
        return SendMessage(SendMessage(observerClass, "alloc"), "init");
    }();

    void* notificationName = MakeString(InFullscreen ? "NSWindowDidEnterFullScreenNotification"
                                                    : "NSWindowDidExitFullScreenNotification");
    void* center = SendMessage(objc_getClass("NSNotificationCenter"), "defaultCenter");

    s_FullscreenTransitionFinished = false;
    ((void (*)(void*, void*, void*, void*, void*, void*))objc_msgSend)(
        center, sel_registerName("addObserver:selector:name:object:"),
        observer, sel_registerName("onTransitionFinished:"), notificationName, InNativeWindow);

    const CFAbsoluteTime deadline = CFAbsoluteTimeGetCurrent() + 2.0;
    while (!s_FullscreenTransitionFinished && CFAbsoluteTimeGetCurrent() < deadline) {
        CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.01, true);
    }

    ((void (*)(void*, void*, void*, void*, void*))objc_msgSend)(
        center, sel_registerName("removeObserver:name:object:"), observer, notificationName, InNativeWindow);

    if (!s_FullscreenTransitionFinished) {
        AE_WARN("Timed out waiting for the window's fullscreen transition");
    }
}

bool MacOSPlatformHooks::SetWindowFullscreen(void* InNativeWindow, bool InFullscreen) {
    if (!InNativeWindow) {
        return false;
    }

    // NSWindowStyleMaskFullScreen. toggleFullScreen: is a toggle with no setter counterpart, so
    // the current state decides whether it needs sending at all.
    static constexpr uint64_t fullScreenStyleMask = 1ull << 14;
    const uint64_t styleMask = ((uint64_t (*)(void*, void*))objc_msgSend)(
        InNativeWindow, sel_registerName("styleMask"));

    if (((styleMask & fullScreenStyleMask) != 0) != InFullscreen) {
        SendMessage(InNativeWindow, "toggleFullScreen:", nullptr);
        WaitForFullscreenTransition(InNativeWindow, InFullscreen);
    }
    return true;
}

static constexpr int32_t s_PreviousInputSourceHotKey = 60;
static constexpr int32_t s_NextInputSourceHotKey = 61;

static void SuppressSystemShortcuts(bool InSuppressed) {
    // CGSSetSymbolicHotKeyEnabled is the only way to release a symbolic hotkey without the user
    // turning it off in System Settings. It is private, so it is resolved at runtime and the
    // suppression is simply skipped if a future release drops it.
    using SetHotKeyEnabledFn = int32_t (*)(int32_t, bool);
    static SetHotKeyEnabledFn setHotKeyEnabled = (SetHotKeyEnabledFn)dlsym(RTLD_DEFAULT, "CGSSetSymbolicHotKeyEnabled");
    static bool suppressed = false;

    if (!setHotKeyEnabled || suppressed == InSuppressed) {
        return;
    }
    if (InSuppressed && !suppressed) {
        // The suspension outlives the process, so make sure a normal exit always restores it.
        static bool registered = false;
        if (!registered) {
            registered = true;
            std::atexit([]() { SuppressSystemShortcuts(false); });
        }
    }
    suppressed = InSuppressed;
    setHotKeyEnabled(s_PreviousInputSourceHotKey, !InSuppressed);
    setHotKeyEnabled(s_NextInputSourceHotKey, !InSuppressed);
}

void MacOSPlatformHooks::SetSystemShortcutsSuppressed(bool InSuppressed) {
    SuppressSystemShortcuts(InSuppressed);
}
