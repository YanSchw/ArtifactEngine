#pragma once
#include "GameFramework/UINode.h"
#include <functional>
#include "UIColorSwatch.gen.h"

class UIColorSwatch : public UINode {
public:
    ARTIFACT_CLASS();

    UIColorSwatch();

    std::function<Color()> Get;
    std::function<void(const Color&)> Set;

    bool UseAlpha = true;
    String Title = "Color";
    float CornerRadius = 3.0f;

    static void PaintColor(UIDrawList& OutDrawList, const UIRectF& InRect, const Color& InColor,
                           float InRadius, const Mat4& InTransform);

    virtual void Paint(UIDrawList& OutDrawList) override;
    virtual void OnClick() override;
};
