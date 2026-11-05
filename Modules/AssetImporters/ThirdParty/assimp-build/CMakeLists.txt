# Configures the vendored assimp for embedding: a static library with a handful of mesh importers
# and no exporters, tools, tests or install rules. Keeping this wrapper separate leaves
# ThirdParty/assimp-6.0.5 pristine.
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_ASSIMP_TOOLS OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_SAMPLES OFF CACHE BOOL "" FORCE)
set(ASSIMP_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INSTALL OFF CACHE BOOL "" FORCE)
set(ASSIMP_NO_EXPORT ON CACHE BOOL "" FORCE)
set(ASSIMP_WARNINGS_AS_ERRORS OFF CACHE BOOL "" FORCE)
set(ASSIMP_INJECT_DEBUG_POSTFIX OFF CACHE BOOL "" FORCE)
set(ASSIMP_IGNORE_GIT_HASH ON CACHE BOOL "" FORCE)

set(ASSIMP_BUILD_ALL_IMPORTERS_BY_DEFAULT OFF CACHE BOOL "" FORCE)
foreach(importer FBX GLTF OBJ COLLADA PLY STL 3DS)
    set(ASSIMP_BUILD_${importer}_IMPORTER ON CACHE BOOL "" FORCE)
endforeach()

# The bundled zlib 1.2.13 defines fdopen away on macOS, which breaks the SDK's own stdio.h, so macOS
# links the SDK's zlib instead. Either way the bundled unzip is used, rather than a system minizip.
if (APPLE)
    set(ASSIMP_BUILD_ZLIB OFF CACHE BOOL "" FORCE)
else()
    set(ASSIMP_BUILD_ZLIB ON CACHE BOOL "" FORCE)
endif()
set(ASSIMP_BUILD_MINIZIP ON)

# The engine passes /utf-8 as a compile option, which MSVC rejects next to the
# /source-charset:utf-8 assimp adds unless it already finds /utf-8 here.
if (MSVC)
    string(APPEND CMAKE_CXX_FLAGS " /utf-8")
endif()

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../assimp-6.0.5 ${CMAKE_CURRENT_BINARY_DIR}/assimp)
