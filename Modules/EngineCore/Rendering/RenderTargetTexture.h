#pragma once
#include "Texture.h"
#include "RenderTargetTexture.gen.h"

/** Adapts a render target's color attachment to the Texture interface so the UI can sample it
 *  (UIImage, image draw batches). Keeps a stable object identity while the underlying view is
 *  swapped on resize. */
class RenderTargetTexture : public Texture {
public:
    ARTIFACT_CLASS();

    void SetView(const SharedObjectPtr<ImageView>& InView) { m_View = InView; }

    virtual SharedObjectPtr<Image> GetImage() const override { return m_View.Get() ? m_View->GetDesc().ImagePtr : nullptr; }
    virtual SharedObjectPtr<ImageView> GetDefaultView() const override { return m_View; }

private:
    SharedObjectPtr<ImageView> m_View;
};
