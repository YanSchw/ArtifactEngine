#include "UIIf.h"

void UIIf::OnBind() {
    UINode::OnBind();

    if (!m_Content) {
        m_Content = Add<UINode>();
        m_Content->Fill();
        if (Build) {
            Build(*m_Content);
        }
    }

    // This node stays enabled so it keeps re-evaluating; only the content is toggled (its binds,
    // layout and paint are skipped while disabled).
    m_Content->SetEnabled(Condition ? Condition() : true);
}
