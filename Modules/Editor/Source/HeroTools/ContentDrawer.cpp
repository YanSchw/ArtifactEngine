#include "ContentDrawer.h"
#include "ThumbnailRenderer.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/UIGrid.h"
#include "Assets/Font.h"
#include "GameFramework/UINode.h"
#include "GameFramework/UIVStack.h"
#include "GameFramework/UIHStack.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UIImage.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UIScrollArea.h"
#include "Assets/AssetManager.h"
#include "Assets/Asset.h"
#include "Assets/VectorImage.h"
#include "Rendering/Texture.h"
#include "Core/EngineConfig.h"
#include "Platform/FileIO.h"
#include "Serialization/ThirdParty/nlohmann/json.hpp"
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <functional>

namespace fs = std::filesystem;

static const Vec4 s_SelectedColor = HexColor(0x0F5A9E);
static const Vec4 s_AddGreen = HexColor(0x3C8C3C);

static void ClearChildren(UINode* InNode) {
    while (InNode->HasChildren()) {
        delete InNode->GetChild(0);
    }
}

static Array<String> SplitPath(const String& InPath) {
    Array<String> parts;
    String current;
    for (char c : InPath) {
        if (c == '/') {
            if (!current.empty()) { parts.Add(current); current.clear(); }
        } else {
            current += c;
        }
    }
    if (!current.empty()) { parts.Add(current); }
    return parts;
}

static Array<String> ListSubfolders(const String& InDir) {
    Array<String> folders;
    std::error_code ec;
    if (fs::exists(InDir, ec) && fs::is_directory(InDir, ec)) {
        for (const fs::directory_entry& entry : fs::directory_iterator(InDir, ec)) {
            if (entry.is_directory(ec)) {
                folders.Add(entry.path().filename().string());
            }
        }
    }
    std::sort(folders.begin(), folders.end());
    return folders;
}

static VectorImage* IconForClass(const String& InClass) {
    if (InClass == "Mesh") return EditorIcons::Mesh();
    if (InClass == "Texture2D") return EditorIcons::Texture();
    if (InClass == "Font") return EditorIcons::Font();
    if (InClass == "VectorImage") return EditorIcons::Node();
    return EditorIcons::Asset();
}

String ContentDrawer::DirFor(const String& InMount, const String& InRel) const {
    String dir = EngineConfig::GetContentDir(InMount);
    if (!InRel.empty()) {
        dir += "/" + InRel;
    }
    return dir;
}

bool ContentDrawer::IsCurrent(const String& InMount, const String& InRel) const {
    return m_Mount == InMount && m_RelPath == InRel;
}

bool ContentDrawer::IsExpanded(const String& InKey) const {
    return m_Expanded.Contains(InKey);
}

void ContentDrawer::SetExpanded(const String& InKey, bool InExpanded) {
    if (InExpanded) {
        if (!m_Expanded.Contains(InKey)) { m_Expanded.Add(InKey); }
    } else {
        m_Expanded.Remove(InKey);
    }
    m_NavDirty = true;
}

void ContentDrawer::NavigateTo(const String& InMount, const String& InRel, bool InPushHistory) {
    m_Mount = InMount;
    m_RelPath = InRel;
    m_SelectedPath.clear();

    // Reveal the target in the tree by expanding its ancestor chain.
    if (!m_Expanded.Contains(InMount)) { m_Expanded.Add(InMount); }
    String accumulated;
    for (const String& segment : SplitPath(InRel)) {
        accumulated = accumulated.empty() ? segment : accumulated + "/" + segment;
        const String key = InMount + "/" + accumulated;
        if (!m_Expanded.Contains(key)) { m_Expanded.Add(key); }
    }

    if (InPushHistory) {
        while (m_History.Size() > m_HistoryPos + 1) {
            m_History.RemoveAt(m_History.Size() - 1);
        }
        m_History.Add({ InMount, InRel });
        m_HistoryPos = m_History.Size() - 1;
    }
    m_NavDirty = true;
}

void ContentDrawer::GoBack() {
    if (m_HistoryPos > 0) {
        m_HistoryPos--;
        NavigateTo(m_History[m_HistoryPos].Mount, m_History[m_HistoryPos].Rel, false);
    }
}

void ContentDrawer::GoForward() {
    if (m_HistoryPos + 1 < m_History.Size()) {
        m_HistoryPos++;
        NavigateTo(m_History[m_HistoryPos].Mount, m_History[m_HistoryPos].Rel, false);
    }
}

void ContentDrawer::BuildDrawer(UINode& InBody) {
    m_Breadcrumb = nullptr;
    m_Tree = nullptr;
    m_Grid = nullptr;

    if (m_Mount.empty()) {
        Array<String> keys = EngineConfig::GetContentMountKeys();
        if (keys.Contains("ProjectContent")) {
            NavigateTo("ProjectContent", "");
        } else if (!keys.IsEmpty()) {
            NavigateTo(keys[0], "");
        }
    }

    UIVStack* root = InBody.Add<UIVStack>();
    root->Fill();

    // --- Toolbar ---
    UIQuad* toolbar = root->Add<UIQuad>();
    toolbar->Size = { 1.0_rel, 38.0_px };
    toolbar->Color = EditorStyle::ToolBar;

    UIHStack* toolbarRow = toolbar->Add<UIHStack>();
    toolbarRow->Fill();
    toolbarRow->Padding = UIPadding(8.0f, 6.0f);
    toolbarRow->Gap = 6.0f;

    const auto labelButton = [](UIStack& InRow, const String& InCaption, float InWidth, const Vec4& InColor,
                                std::function<void()> InOnClick) {
        UIButton* button = InRow.Add<UIButton>();
        button->Size = { UIValue(InWidth), 1.0_rel };
        button->SetCaption(InCaption);
        EditorStyle::ApplyButtonStyle(*button);
        button->NormalColor = InColor;
        button->Clicked = std::move(InOnClick);
        return button;
    };
    const auto arrowButton = [](UIStack& InRow, VectorImage* InIcon, float InRotation, std::function<void()> InOnClick) {
        UIButton* button = InRow.Add<UIButton>();
        button->Size = { 26.0_px, 1.0_rel };
        button->NormalColor = EditorStyle::Button;
        button->HoverColor = EditorStyle::ButtonHover;
        button->PressedColor = EditorStyle::ButtonPressed;
        button->Clicked = std::move(InOnClick);
        UISvg* icon = button->Add<UISvg>();
        icon->Center(Vec2(11.0f, 11.0f));
        icon->Image = InIcon;
        icon->Tint = EditorStyle::Text;
        icon->Rotation = Vec3(0.0f, 0.0f, InRotation);
    };

    labelButton(*toolbarRow, "+ Add", 62.0f, s_AddGreen, [] {});
    labelButton(*toolbarRow, "Import", 62.0f, EditorStyle::Button, [] {});
    labelButton(*toolbarRow, "Save All", 74.0f, EditorStyle::Button, [] {});

    arrowButton(*toolbarRow, EditorIcons::ArrowRight(), 180.0f, [this] { GoBack(); });
    arrowButton(*toolbarRow, EditorIcons::ArrowRight(), 0.0f, [this] { GoForward(); });

    m_Breadcrumb = toolbarRow->Add<UIHStack>();
    m_Breadcrumb->Size = { 1.0_rel, 1.0_rel };
    m_Breadcrumb->Gap = 2.0f;

    // --- Body: tree | grid ---
    UIHStack* body = root->Add<UIHStack>();
    body->Size = { 1.0_rel, 1.0_rel };

    UIQuad* leftPanel = body->Add<UIQuad>();
    leftPanel->Size = { 230.0_px, 1.0_rel };
    leftPanel->Color = EditorStyle::PanelDark;

    UIScrollArea* treeScroll = leftPanel->Add<UIScrollArea>();
    treeScroll->Fill();
    UIVStack* tree = treeScroll->Add<UIVStack>();
    tree->Fill();
    tree->Padding = UIPadding(2.0f, 4.0f);
    m_Tree = tree;

    UIQuad* rightPanel = body->Add<UIQuad>();
    rightPanel->Size = { 1.0_rel, 1.0_rel };
    rightPanel->Color = EditorStyle::Panel;

    UIScrollArea* gridScroll = rightPanel->Add<UIScrollArea>();
    gridScroll->Fill();
    UIGrid* grid = gridScroll->Add<UIGrid>();
    grid->Fill();
    grid->Padding = UIPadding(12.0f, 12.0f);
    grid->CellSize = Vec2(104.0f, 128.0f);
    grid->Gap = Vec2(12.0f, 14.0f);
    m_Grid = grid;

    m_NavDirty = true;
}

void ContentDrawer::Tick(float InDeltaTime) {
    (void)InDeltaTime;
    if (m_NavDirty) {
        m_NavDirty = false;
        RebuildContent();
    }
    if (m_Thumbnails.Get()) {
        m_Thumbnails->Tick();
    }
}

void ContentDrawer::RebuildContent() {
    if (!m_Breadcrumb || !m_Tree || !m_Grid) {
        return;
    }
    BuildBreadcrumb();
    BuildTree();
    BuildGrid();
}

void ContentDrawer::BuildBreadcrumb() {
    ClearChildren(m_Breadcrumb);

    Array<String> segments;
    segments.Add(m_Mount);
    for (const String& folder : SplitPath(m_RelPath)) {
        segments.Add(folder);
    }

    for (int32_t i = 0; i < segments.Size(); i++) {
        if (i > 0) {
            UISvg* chevron = m_Breadcrumb->Add<UISvg>();
            chevron->Size = { 12.0_px, 1.0_rel };
            chevron->Image = EditorIcons::ArrowRight();
            chevron->Tint = EditorStyle::TextDim;
        }

        String targetRel;
        for (int32_t j = 1; j <= i; j++) {
            targetRel += (j == 1 ? "" : "/") + segments[j];
        }

        UIButton* crumb = m_Breadcrumb->Add<UIButton>();
        crumb->Size = { 12.0_px + UINode::GetDefaultFont()->GetTextWidth(segments[i], EditorStyle::FontSize), 1.0_rel };
        crumb->NormalColor = Vec4(0.0f);
        crumb->HoverColor = EditorStyle::TabHover;
        crumb->PressedColor = EditorStyle::ButtonPressed;
        const String mount = m_Mount;
        crumb->Clicked = [this, mount, targetRel] { NavigateTo(mount, targetRel); };

        UILabel* label = crumb->Add<UILabel>();
        label->Fill();
        label->Padding = UIPadding(6.0f, 0.0f);
        label->FontSize = EditorStyle::FontSize;
        label->VAlign = UIVAlign::Middle;
        label->Color = (i == segments.Size() - 1) ? EditorStyle::TextBright : EditorStyle::TextDim;
        label->Text = segments[i];
    }
}

void ContentDrawer::BuildTree() {
    ClearChildren(m_Tree);
    for (const String& mount : EngineConfig::GetContentMountKeys()) {
        BuildTreeNode(mount, "", mount, 0);
    }
}

void ContentDrawer::BuildTreeNode(const String& InMount, const String& InRel, const String& InName, int32_t InDepth) {
    const String key = InRel.empty() ? InMount : InMount + "/" + InRel;
    const Array<String> subfolders = ListSubfolders(DirFor(InMount, InRel));
    const bool hasChildren = !subfolders.IsEmpty();
    const bool expanded = IsExpanded(key);
    const float indent = 6.0f + (float)InDepth * 14.0f;

    UIButton* row = m_Tree->Add<UIButton>();
    row->Size = { 1.0_rel, 22.0_px };
    row->HoverColor = EditorStyle::TabHover;
    row->PressedColor = EditorStyle::ButtonPressed;
    const String mount = InMount;
    const String rel = InRel;
    row->Clicked = [this, mount, rel] { NavigateTo(mount, rel); };
    row->Bind = [this, row, mount, rel] {
        row->NormalColor = IsCurrent(mount, rel) ? s_SelectedColor : Vec4(0.0f);
    };

    if (hasChildren) {
        UIButton* toggle = row->Add<UIButton>();
        toggle->Anchor = toggle->Pivot = Vec2(0.0f, 0.5f);
        toggle->Position = Vec2(indent - 2.0f, 0.0f);
        toggle->Size = Vec2(16.0f, 22.0f);
        toggle->NormalColor = Vec4(0.0f);
        toggle->HoverColor = Vec4(0.0f);
        toggle->PressedColor = Vec4(0.0f);
        toggle->Clicked = [this, key, expanded] { SetExpanded(key, !expanded); };
        UISvg* arrow = toggle->Add<UISvg>();
        arrow->Center(Vec2(9.0f, 9.0f));
        arrow->Image = expanded ? EditorIcons::ArrowDown() : EditorIcons::ArrowRight();
        arrow->Tint = EditorStyle::TextDim;
    }

    UISvg* folderIcon = row->Add<UISvg>();
    folderIcon->Anchor = folderIcon->Pivot = Vec2(0.0f, 0.5f);
    folderIcon->Position = Vec2(indent + 15.0f, 0.0f);
    folderIcon->Size = Vec2(15.0f, 15.0f);
    folderIcon->Image = EditorIcons::Folder();
    folderIcon->Tint = EditorStyle::Folder;

    UILabel* label = row->Add<UILabel>();
    label->Anchor = label->Pivot = Vec2(0.0f, 0.5f);
    label->Position = Vec2(indent + 34.0f, 0.0f);
    label->Size = { 1.0_rel - UIValue::Px(indent + 38.0f), 22.0_px };
    label->FontSize = EditorStyle::FontSize;
    label->VAlign = UIVAlign::Middle;
    label->Color = IsCurrent(InMount, InRel) ? EditorStyle::TextBright : EditorStyle::Text;
    label->Text = InName;

    if (expanded) {
        for (const String& sub : subfolders) {
            const String childRel = InRel.empty() ? sub : InRel + "/" + sub;
            BuildTreeNode(InMount, childRel, sub, InDepth + 1);
        }
    }
}

void ContentDrawer::BuildGrid() {
    ClearChildren(m_Grid);

    struct GridEntry {
        bool IsFolder = false;
        String Name;
        String Rel;
        String FilePath;
        String AssetClass;
        Asset* AssetPtr = nullptr;
    };
    Array<GridEntry> entries;

    const String dir = DirFor(m_Mount, m_RelPath);
    for (const String& sub : ListSubfolders(dir)) {
        GridEntry folder;
        folder.IsFolder = true;
        folder.Name = sub;
        folder.Rel = m_RelPath.empty() ? sub : (m_RelPath + "/" + sub);
        entries.Add(folder);
    }

    std::error_code ec;
    if (fs::exists(dir, ec) && fs::is_directory(dir, ec)) {
        for (const fs::directory_entry& entry : fs::directory_iterator(dir, ec)) {
            if (!entry.is_directory(ec) && entry.path().extension().string() == ".asset") {
                GridEntry asset;
                asset.FilePath = entry.path().string();
                asset.Name = entry.path().stem().string();
                try {
                    nlohmann::json json = nlohmann::json::parse(FileIO::ReadFileToString(asset.FilePath));
                    if (json.contains("AssetClass")) {
                        asset.AssetClass = json["AssetClass"].get<String>();
                    }
                    if (json.contains("m_Id")) {
                        asset.AssetPtr = AssetManager::Get().GetAsset(UUID::FromString(json["m_Id"].get<String>()));
                    }
                } catch (...) {
                }
                entries.Add(asset);
            }
        }
    }

    std::sort(entries.begin(), entries.end(), [](const GridEntry& InA, const GridEntry& InB) {
        if (InA.IsFolder != InB.IsFolder) {
            return InA.IsFolder;
        }
        return InA.Name < InB.Name;
    });

    for (const GridEntry& entry : entries) {
        UIButton* card = m_Grid->Add<UIButton>();
        card->NormalColor = Vec4(0.0f);
        card->PressedColor = Vec4(0.0f);

        if (entry.IsFolder) {
            const String rel = entry.Rel;
            const String mount = m_Mount;
            card->HoverColor = Vec4(1.0f, 1.0f, 1.0f, 0.05f);
            card->Clicked = [this, mount, rel] { NavigateTo(mount, rel); };

            UISvg* folderIcon = card->Add<UISvg>();
            folderIcon->Anchor = folderIcon->Pivot = Vec2(0.5f, 0.0f);
            folderIcon->Position = Vec2(0.0f, 16.0f);
            folderIcon->Size = { 1.0_rel - 22.0_px, 58.0_px };
            folderIcon->Image = EditorIcons::Folder();
            folderIcon->Tint = EditorStyle::Folder;

            UILabel* name = card->Add<UILabel>();
            name->Anchor = name->Pivot = Vec2(0.5f, 1.0f);
            name->Position = Vec2(0.0f, -6.0f);
            name->Size = { 1.0_rel, 26.0_px };
            name->FontSize = EditorStyle::FontSize - 1.0f;
            name->HAlign = UIHAlign::Center;
            name->VAlign = UIVAlign::Middle;
            name->Color = EditorStyle::Text;
            name->Text = entry.Name;
            continue;
        }

        // Asset card: a framed thumbnail with a name band + type strip, like Unreal's content browser.
        const String filePath = entry.FilePath;
        card->HoverColor = Vec4(0.0f);
        card->Clicked = [this, filePath] { m_SelectedPath = filePath; };

        UIQuad* frame = card->Add<UIQuad>();
        frame->Fill();
        frame->Bind = [this, card, frame, filePath] {
            const bool selected = (m_SelectedPath == filePath);
            frame->Color = selected ? s_SelectedColor
                                    : (card->IsHovered() ? HexColor(0x3C3C3C) : HexColor(0x2A2A2A));
        };

        UIVStack* column = frame->Add<UIVStack>();
        column->Fill();
        column->Padding = UIPadding(1.0f, 1.0f);

        UIQuad* thumbBg = column->Add<UIQuad>();
        thumbBg->Size = { 1.0_rel, 1.0_rel };
        thumbBg->Color = EditorStyle::PanelDark;

        UISvg* icon = thumbBg->Add<UISvg>();
        icon->Center(Vec2(40.0f, 40.0f));
        icon->Image = IconForClass(entry.AssetClass);
        icon->Tint = EditorStyle::Text;

        if (entry.AssetPtr) {
            if (!m_Thumbnails.Get()) {
                m_Thumbnails = Object::Create<ThumbnailRenderer>();
            }
            if (Texture* thumbnail = m_Thumbnails->GetThumbnail(entry.AssetPtr)) {
                UIImage* preview = thumbBg->Add<UIImage>();
                preview->Center({ 1.0_rel - 10.0_px, 1.0_rel - 10.0_px });
                preview->Image = thumbnail;
                preview->Bind = [preview, icon, thumbnail] {
                    const bool ready = thumbnail->GetDefaultView().Get() != nullptr;
                    preview->SetEnabled(ready);
                    icon->SetEnabled(!ready);
                };
            }
        }

        UIQuad* nameBand = column->Add<UIQuad>();
        nameBand->Size = { 1.0_rel, 22.0_px };
        nameBand->Bind = [this, nameBand, filePath] {
            nameBand->Color = (m_SelectedPath == filePath) ? s_SelectedColor : EditorStyle::Panel;
        };
        UILabel* name = nameBand->Add<UILabel>();
        name->Fill();
        name->Padding = UIPadding(6.0f, 0.0f);
        name->FontSize = EditorStyle::FontSize - 1.0f;
        name->VAlign = UIVAlign::Middle;
        name->Color = EditorStyle::TextBright;
        name->Text = entry.Name;

        UIQuad* typeStrip = column->Add<UIQuad>();
        typeStrip->Size = { 1.0_rel, 15.0_px };
        typeStrip->Color = EditorStyle::PanelDark;
        UILabel* type = typeStrip->Add<UILabel>();
        type->Fill();
        type->Padding = UIPadding(6.0f, 0.0f);
        type->FontSize = EditorStyle::FontSize - 3.0f;
        type->VAlign = UIVAlign::Middle;
        type->Color = EditorStyle::AccentBright;
        type->Text = entry.AssetClass.empty() ? String("Asset") : entry.AssetClass;
    }
}
