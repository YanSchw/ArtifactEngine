# Configures the vendored msdfgen for embedding: core library only, so it pulls in no
# external dependencies (no FreeType, Skia, vcpkg, PNG/SVG, or standalone executable).
# The glyph outlines are supplied directly from stb_truetype, so the FreeType-backed
# ext library is never needed. Keeping this wrapper separate leaves ThirdParty/msdfgen pristine.
set(MSDFGEN_CORE_ONLY ON CACHE BOOL "" FORCE)
set(MSDFGEN_BUILD_STANDALONE OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_VCPKG OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_OPENMP OFF CACHE BOOL "" FORCE)
set(MSDFGEN_USE_SKIA OFF CACHE BOOL "" FORCE)
set(MSDFGEN_INSTALL OFF CACHE BOOL "" FORCE)
set(MSDFGEN_DYNAMIC_RUNTIME OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../msdfgen ${CMAKE_CURRENT_BINARY_DIR}/msdfgen)
