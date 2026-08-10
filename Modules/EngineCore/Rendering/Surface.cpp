#include "Surface.h"

#include "Platform/Platform.h"

static Array<Surface*> s_AllSurfaces;

Surface::Surface() {
    s_AllSurfaces.Add(this);
}

Surface::~Surface() {
    s_AllSurfaces.Remove(this);
}

SharedObjectPtr<Surface> Surface::CreateMain(const SurfaceParams& InParams) {
    SharedObjectPtr<Surface> surface = Object::Create<Surface>(Platform::GetMainSurfaceClass());
    AE_ASSERT(surface, "Failed to create the main surface");
    surface->Initialize(InParams);
    return surface;
}

Surface* Surface::GetMain() {
    return s_AllSurfaces.IsEmpty() ? nullptr : s_AllSurfaces[0];
}
