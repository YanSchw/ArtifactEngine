#include "MajorTab.h"
#include "UI/UIDockArea.h"

MajorTab::MajorTab() {
    Fill();
    m_DockArea = Add<UIDockArea>();
}
