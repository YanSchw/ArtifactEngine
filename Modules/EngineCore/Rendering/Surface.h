#pragma once
#include "Core/Object.h"
#include "Core/Pointer.h"
#include "Common/Types.h"
#include "Surface.gen.h"

/* A platform-agnostic surface class for rendering */
class Surface : public Object {
public:
    ARTIFACT_CLASS();
    virtual ~Surface() = default;

    virtual uint32_t GetWidth() const = 0;
    virtual uint32_t GetHeight() const = 0;
};