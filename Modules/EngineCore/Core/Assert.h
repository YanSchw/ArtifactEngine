#pragma once

#include <filesystem>

#define AE_ENABLE_ASSERTS
#define AE_ENABLE_VERIFY

// Debugbreak
#if defined(_WIN32)
    #define AE_DEBUGBREAK() __debugbreak()
#elif defined(__APPLE__) || defined(__linux__)
    #include <signal.h>
    #define AE_DEBUGBREAK() raise(SIGTRAP)
#else
    #warning "Platform doesn't support debugbreak!"
    #define AE_DEBUGBREAK()
#endif


#if defined(AE_ENABLE_ASSERTS) || defined(AE_ENABLE_VERIFY)

    #define AE_INTERNAL_ASSERT_IMPL(check, msg, ...) { if(!(check)) { AE_ERROR(msg, __VA_ARGS__); AE_DEBUGBREAK(); } }
    #define AE_INTERNAL_ASSERT_WITH_MSG(check, ...) AE_INTERNAL_ASSERT_IMPL(check, "Assertion failed: {0}", __VA_ARGS__)
    #define AE_INTERNAL_ASSERT_NO_MSG(check) AE_INTERNAL_ASSERT_IMPL(check, "Assertion '{0}' failed at {1}:{2}", AE_STRINGIFY_MACRO(check), std::filesystem::path(__FILE__).filename().string(), __LINE__)

    #define AE_INTERNAL_ASSERT_GET_MACRO_NAME(arg1, arg2, macro, ...) macro
    #define AE_INTERNAL_ASSERT_GET_MACRO(...) AE_EXPAND_MACRO( AE_INTERNAL_ASSERT_GET_MACRO_NAME(__VA_ARGS__, AE_INTERNAL_ASSERT_WITH_MSG, AE_INTERNAL_ASSERT_NO_MSG) )

#endif

#define AE_EXPAND_MACRO(x) x
#define AE_STRINGIFY_MACRO(x) #x

#ifdef AE_ENABLE_ASSERTS
    #define AE_ASSERT(...) AE_EXPAND_MACRO( AE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#else
    #define AE_ASSERT(...)
#endif

#ifdef AE_ENABLE_VERIFY
    #define AE_VERIFY(...) AE_EXPAND_MACRO( AE_INTERNAL_ASSERT_GET_MACRO(__VA_ARGS__)(__VA_ARGS__) )
#else
    #define AE_VERIFY(...)
#endif