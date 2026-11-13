#pragma once
#include "Rendering/Surface.h"
#include "iOSBridge.h"
#include "iOSSurface.gen.h"

class iOSSurface : public Surface {
public:
    ARTIFACT_CLASS();

    virtual ~iOSSurface();

    virtual void Initialize(const SurfaceParams& InParams) override;

    virtual uint32_t GetWidth() const override;
    virtual uint32_t GetHeight() const override;
    virtual bool IsMinimized() const override;
    virtual void ProcessEvents() override;

    void* GetMetalLayer() const;

    void QueueTouch(const iOSTouchEvent& InEvent);
    Array<iOSTouchEvent> ConsumeTouchEvents();

    static iOSSurface* Get();

private:
    Array<iOSTouchEvent> m_TouchEvents;
};
