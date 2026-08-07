#include "ContentDrawer.h"
#include "ContentTile.h"
#include "ThumbnailRenderer.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/UIGrid.h"
#include "UI/UIRoundedQuad.h"
#include "UI/UIModalDialog.h"
#include "UI/EditorDragDrop.h"
#include "Assets/Font.h"
#include "GameFramework/UINode.h"
#include "GameFramework/UIVStack.h"
#include "GameFramework/UIHStack.h"
#include "GameFramework/UIQuad.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UISvg.h"
#include "GameFramework/UIImage.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UITextArea.h"
#include "GameFramework/UIScrollArea.h"
#include "Assets/AssetManager.h"
#include "Assets/Asset.h"
#include "Assets/VectorImage.h"
#include "Assets/Scene.h"
#include "Assets/Blueprint.h"
#include "Assets/ShaderGraph.h"
#include "Assets/NodeRecord.h"
#include "GameFramework/Node3D.h"
#include "EditorWindow.h"
#include "Tabs/SceneEditorTab.h"
#include "Tabs/BlueprintEditorTab.h"
#include "UI/UIContextMenu.h"
#include "UI/UIMenuModel.h"
#include "Rendering/Texture.h"
#include "Core/EngineConfig.h"
#include "Core/Log.h"
#include "Platform/FileIO.h"
#include "Serialization/ThirdParty/nlohmann/json.hpp"
#include <filesystem>
#include <algorithm>
#include <cctype>
#include <functional>
#include <memory>

namespace fs = std::filesystem;

static const Vec4 s_SelectedColor = HexColor(0x0F5A9E);
static const Vec4 s_AddGreen = HexColor(0x3C8C3C);
static const Vec4 s_CardBase = HexColor(0x2A2A2A);
static const Vec4 s_CardHover = HexColor(0x3C3C3C);
static const Vec4 s_ThumbBackground = HexColor(0x141414);
static const Vec4 s_DropColor = HexColor(0x26BBFF, 0.35f);

static constexpr float s_CardWidth = 104.0f;
static constexpr float s_CardHeight = 128.0f;
static constexpr float s_TypeBarHeight = 3.0f;
static constexpr float s_NameRowHeight = 20.0f;
static constexpr float s_TypeRowHeight = 15.0f;
static constexpr float s_LabelInset = 5.0f;

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

/** Trims the text with an ellipsis so it never paints past the width it was given. */
static String Ellipsize(const String& InText, float InFontSize, float InMaxWidth) {
    Font* font = UINode::GetDefaultFont();
    if (!font || InMaxWidth <= 0.0f || font->GetTextWidth(InText, InFontSize) <= InMaxWidth) {
        return InText;
    }
    String trimmed = InText;
    while (!trimmed.empty() && font->GetTextWidth(trimmed + "...", InFontSize) > InMaxWidth) {
        trimmed.pop_back();
    }
    return trimmed + "...";
}

/** Strips whatever would turn a typed name into a path or an invalid file name. */
static String SanitizeName(const String& InName) {
    String name;
    for (char c : InName) {
        if (c == '/' || c == '\\' || c == ':' || c == '*' || c == '?' || c == '"' || c == '<' || c == '>' || c == '|') {
            continue;
        }
        name += c;
    }
    while (!name.empty() && std::isspace((unsigned char)name.front())) { name.erase(name.begin()); }
    while (!name.empty() && std::isspace((unsigned char)name.back())) { name.pop_back(); }
    return name;
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
    CancelEdit();

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
    m_DropTargets.Clear();
    m_Dragging = false;
    CancelEdit();

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

    UIButton* addButton = labelButton(*toolbarRow, "+ Add", 62.0f, s_AddGreen, [] {});
    addButton->Clicked = [this, addButton] {
        UIMenuModel menu;
        BuildAddMenu(menu);
        UIContextMenu::OpenUnder(*addButton, menu);
    };
    labelButton(*toolbarRow, "Import", 62.0f, EditorStyle::Button, [] {});
    labelButton(*toolbarRow, "Save All", 74.0f, EditorStyle::Button, [this] {
        EditorWindow* window = GetOwnerWindow();
        for (MajorTab* tab : window ? window->GetOpenTabs() : Array<MajorTab*>()) {
            if (SceneEditorTab* sceneTab = tab->As<SceneEditorTab>()) {
                sceneTab->Save();
            } else if (BlueprintEditorTab* blueprintTab = tab->As<BlueprintEditorTab>()) {
                blueprintTab->Save();
            }
        }
    });

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

    // The panel doubles as the grid's empty-space hit target: a right-click on nothing bubbles
    // out of the scroll area and lands here.
    UIButton* rightPanel = body->Add<UIButton>();
    rightPanel->Size = { 1.0_rel, 1.0_rel };
    rightPanel->NormalColor = EditorStyle::Panel;
    rightPanel->HoverColor = EditorStyle::Panel;
    rightPanel->PressedColor = EditorStyle::Panel;
    rightPanel->Cursor = CursorIcon::Arrow;
    rightPanel->Clicked = [this] { m_SelectedPath.clear(); };
    rightPanel->SecondaryClicked = [this, rightPanel](const Vec2& InCursorPos) {
        UIMenuModel menu;
        BuildAddMenu(menu);
        UIContextMenu::OpenAt(*rightPanel, InCursorPos, menu);
    };

    UIScrollArea* gridScroll = rightPanel->Add<UIScrollArea>();
    gridScroll->Fill();
    UIGrid* grid = gridScroll->Add<UIGrid>();
    grid->Fill();
    grid->Padding = UIPadding(12.0f, 12.0f);
    grid->CellSize = Vec2(s_CardWidth, s_CardHeight);
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
    m_DropTargets.Clear();
    m_DropNode = nullptr;
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
        crumb->Bind = [this, crumb] { crumb->NormalColor = IsDropHighlight(*crumb) ? s_DropColor : Vec4(0.0f); };
        RegisterDropTarget(*crumb, mount, targetRel);

        UILabel* label = crumb->Add<UILabel>();
        label->Fill();
        label->Padding = UIPadding(6.0f, 0.0f);
        label->FontSize = EditorStyle::FontSize;
        label->VAlign = UIVAlign::Middle;
        label->Color = (i == segments.Size() - 1) ? EditorStyle::TextBright : EditorStyle::TextDim;
        label->Text = segments[i];
    }
}

String ContentDrawer::MakeUniqueName(const String& InDir, const String& InBaseName, bool InFolder) const {
    const auto taken = [&](const String& InName) {
        return fs::exists(InDir + "/" + InName + (InFolder ? "" : ".asset"));
    };
    String name = InBaseName;
    int32_t suffix = 1;
    while (taken(name)) {
        name = InBaseName + std::to_string(++suffix);
    }
    return name;
}

void ContentDrawer::OpenAsset(Asset* InAsset) {
    EditorWindow* window = GetOwnerWindow();
    if (!window) {
        return;
    }
    if (window->OpenAssetEditor(InAsset)) {
        window->CloseHeroTool();
    }
}

/* -------------------------------- Menus -------------------------------- */

void ContentDrawer::BuildAddMenu(UIMenuModel& OutMenu) {
    OutMenu.Section("Create");
    OutMenu.Item("New Folder", [this] { BeginCreateFolder(); }).Icon(EditorIcons::Folder());

    OutMenu.Separator();
    OutMenu.Section("Create Asset");
    OutMenu.Item("Scene", [this] {
        BeginCreateAsset(Scene::StaticClass(), "NewScene", [](const String& InDir, const String& InName) {
            return (Asset*)Scene::CreateEmpty(InDir, InName);
        });
    }).Icon(EditorIcons::Level());
    OutMenu.Item("Blueprint...", [this] { OpenNewBlueprintDialog(); })
           .Icon(EditorIcons::Node())
           .Tooltip("Pick a parent class and a name for the new Blueprint");
    OutMenu.Item("Shader Graph", [this] {
        BeginCreateAsset(ShaderGraph::StaticClass(), "NewShaderGraph", [](const String& InDir, const String& InName) {
            return (Asset*)ShaderGraph::CreateEmpty(InDir, InName);
        });
    }).Icon(EditorIcons::GraphEditor());
}

void ContentDrawer::BuildItemMenu(UIMenuModel& OutMenu, const Item& InItem) {
    const Item item = InItem;

    OutMenu.Section(item.Name);
    OutMenu.Item("Open", [this, item] {
        if (item.IsFolder) {
            NavigateTo(m_Mount, item.Rel);
        } else if (item.AssetPtr) {
            OpenAsset(item.AssetPtr);
        }
    }).Enabled(item.IsFolder || item.AssetPtr != nullptr)
      .Icon(item.IsFolder ? EditorIcons::Folder() : EditorIcons::GetAssetIcon(item.AssetClass));
    OutMenu.Item("Rename", [this, item] { BeginRename(item); }).Shortcut("F2");
    OutMenu.Separator();
    OutMenu.Submenu("Move To", [this, item](UIMenuModel& OutSub) {
        for (const String& mount : EngineConfig::GetContentMountKeys()) {
            OutSub.Section(mount);
            const String mountKey = mount;
            OutSub.Item("(root)", [this, item, mountKey] { MoveItem(item, mountKey, ""); }).Icon(EditorIcons::Folder());
            for (const String& folder : ListSubfolders(DirFor(mount, ""))) {
                OutSub.Item(folder, [this, item, mountKey, folder] { MoveItem(item, mountKey, folder); })
                      .Icon(EditorIcons::Folder());
            }
        }
    }).Tooltip("Dragging a card onto a folder moves it too");
    OutMenu.Separator();
    OutMenu.Item("Delete", [this, item] { DeleteItem(item); })
           .Tooltip(item.IsFolder ? "Deletes the folder and everything in it" : "Deletes the asset file");
}

void ContentDrawer::BuildFolderMenu(UIMenuModel& OutMenu, const String& InMount, const String& InRel) {
    const String mount = InMount;
    const String rel = InRel;
    const bool isRoot = IsMountRoot(rel);

    OutMenu.Section(rel.empty() ? mount : SplitPath(rel).LastItem());
    OutMenu.Item("New Folder", [this, mount, rel] {
        NavigateTo(mount, rel);
        BeginCreateFolder();
    }).Icon(EditorIcons::Folder());

    if (isRoot) {
        return;
    }

    Item item;
    item.IsFolder = true;
    item.Rel = rel;
    item.Name = SplitPath(rel).LastItem();
    item.Path = DirFor(mount, rel);

    const Array<String> segments = SplitPath(rel);
    String parentRel;
    for (int32_t i = 0; i + 1 < segments.Size(); i++) {
        parentRel += (i == 0 ? "" : "/") + segments[i];
    }

    OutMenu.Separator();
    // Renaming happens on the card, so the parent folder has to be on screen first.
    OutMenu.Item("Rename", [this, mount, parentRel, item] {
        NavigateTo(mount, parentRel);
        BeginRename(item);
    }).Shortcut("F2");
    OutMenu.Item("Delete", [this, item] { DeleteItem(item); })
           .Tooltip("Deletes the folder and everything in it");
}

/* -------------------------------- Tree -------------------------------- */

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
    row->SecondaryClicked = [this, row, mount, rel](const Vec2& InCursorPos) {
        UIMenuModel menu;
        BuildFolderMenu(menu, mount, rel);
        UIContextMenu::OpenAt(*row, InCursorPos, menu);
    };
    row->Bind = [this, row, mount, rel] {
        row->NormalColor = IsDropHighlight(*row) ? s_DropColor
                         : (IsCurrent(mount, rel) ? s_SelectedColor : Vec4(0.0f));
    };
    RegisterDropTarget(*row, mount, rel);

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

/* -------------------------------- Grid -------------------------------- */

Array<ContentDrawer::Item> ContentDrawer::CollectItems() const {
    Array<Item> items;

    const String dir = DirFor(m_Mount, m_RelPath);
    for (const String& sub : ListSubfolders(dir)) {
        Item folder;
        folder.IsFolder = true;
        folder.Name = sub;
        folder.Rel = m_RelPath.empty() ? sub : (m_RelPath + "/" + sub);
        folder.Path = dir + "/" + sub;
        items.Add(folder);
    }

    std::error_code ec;
    if (fs::exists(dir, ec) && fs::is_directory(dir, ec)) {
        for (const fs::directory_entry& entry : fs::directory_iterator(dir, ec)) {
            if (entry.is_directory(ec) || entry.path().extension().string() != ".asset") {
                continue;
            }
            Item asset;
            asset.Path = entry.path().string();
            asset.Name = entry.path().stem().string();
            try {
                nlohmann::json json = nlohmann::json::parse(FileIO::ReadFileToString(asset.Path));
                if (json.contains("AssetClass")) {
                    asset.AssetClass = Class(json["AssetClass"].get<String>());
                }
                if (json.contains("m_Id")) {
                    asset.AssetPtr = AssetManager::Get().GetAsset(UUID::FromString(json["m_Id"].get<String>()));
                }
            } catch (...) {
            }
            items.Add(asset);
        }
    }

    std::sort(items.begin(), items.end(), [](const Item& InA, const Item& InB) {
        if (InA.IsFolder != InB.IsFolder) {
            return InA.IsFolder;
        }
        return InA.Name < InB.Name;
    });
    return items;
}

void ContentDrawer::BuildGrid() {
    ClearChildren(m_Grid);

    Array<Item> items = CollectItems();

    // A new item has no file yet; it rides along as a card whose name row is the edit field.
    if (m_Edit.Active && m_Edit.Path.empty()) {
        Item placeholder;
        placeholder.IsFolder = m_Edit.IsFolder;
        placeholder.Name = m_Edit.Name;
        placeholder.AssetClass = m_Edit.AssetClass;
        items.Insert(m_Edit.IsFolder ? 0 : items.Size(), placeholder);
    }

    for (const Item& item : items) {
        if (item.IsFolder) {
            BuildFolderTile(item);
        } else {
            BuildAssetTile(item);
        }
    }
}

UITextArea* ContentDrawer::AddEditField(UINode& InParent) {
    UITextArea* field = InParent.Add<UITextArea>();
    field->Fill();
    field->Padding = UIPadding(3.0f, 0.0f);
    field->SingleLine = true;
    field->FontSize = EditorStyle::FontSize - 1.0f;
    field->TextColor = EditorStyle::TextBright;
    field->CaretColor = EditorStyle::TextBright;
    field->BackgroundColor = EditorStyle::PanelDark;
    field->FocusedBorderColor = EditorStyle::Accent;
    field->CornerRadius = 2.0f;
    field->Text = m_Edit.Name;
    // Keep the pending name in the edit state, so a rebuild mid-edit restores what was typed.
    field->TextChanged = [this](const String& InText) { m_Edit.Name = InText; };
    field->Submitted = [this](const String& InName) { CommitEdit(InName); };
    field->Cancelled = [this] { CancelEdit(); };
    field->FocusLost = [this, field] {
        // Commit-on-blur; a no-op once Submitted or Cancelled already ended this edit.
        if (m_Edit.Active) {
            CommitEdit(field->Text);
        }
    };
    field->SelectAll();
    field->RequestFocus();
    return field;
}

void ContentDrawer::BuildFolderTile(const Item& InItem) {
    const Item item = InItem;
    const bool placeholder = item.Path.empty();
    const bool editing = IsEditing(item);

    ContentTile* card = m_Grid->Add<ContentTile>();
    card->CornerRadius = 4.0f;
    card->ClipChildren = true;
    card->NormalColor = card->PressedColor = Vec4(0.0f);
    card->HoverColor = Vec4(1.0f, 1.0f, 1.0f, 0.05f);

    if (!placeholder) {
        const String mount = m_Mount;
        card->Clicked = [this, item] { m_SelectedPath = item.Path; };
        card->DoubleClicked = [this, mount, item] { NavigateTo(mount, item.Rel); };
        card->SecondaryClicked = [this, card, item](const Vec2& InCursorPos) {
            m_SelectedPath = item.Path;
            UIMenuModel menu;
            BuildItemMenu(menu, item);
            UIContextMenu::OpenAt(*card, InCursorPos, menu);
        };
        card->DragStarted = [this, item] { BeginDrag(item); };
        card->DragMoved = [this](const Vec2& InCursorPos) { DragOver(InCursorPos); };
        card->DragEnded = [this] { EndDrag(); };
        card->Bind = [this, card, item] {
            const bool selected = (m_SelectedPath == item.Path);
            const Vec4 highlight = IsDropHighlight(*card) ? s_DropColor : (selected ? s_SelectedColor : Vec4(0.0f));
            card->NormalColor = highlight;
            card->HoverColor = (highlight.a > 0.0f) ? highlight : Vec4(1.0f, 1.0f, 1.0f, 0.05f);
        };
        RegisterDropTarget(*card, m_Mount, item.Rel);
    }

    UISvg* folderIcon = card->Add<UISvg>();
    folderIcon->Anchor = folderIcon->Pivot = Vec2(0.5f, 0.0f);
    folderIcon->Position = Vec2(0.0f, 16.0f);
    folderIcon->Size = { 1.0_rel - 22.0_px, 58.0_px };
    folderIcon->Image = EditorIcons::Folder();
    folderIcon->Tint = EditorStyle::Folder;

    UINode* nameRow = card->Add<UINode>();
    nameRow->Anchor = nameRow->Pivot = Vec2(0.5f, 1.0f);
    nameRow->Position = Vec2(0.0f, -6.0f);
    nameRow->Size = { 1.0_rel - 8.0_px, UIValue(s_NameRowHeight) };

    if (editing) {
        AddEditField(*nameRow);
        return;
    }

    UILabel* name = nameRow->Add<UILabel>();
    name->Fill();
    name->FontSize = EditorStyle::FontSize - 1.0f;
    name->HAlign = UIHAlign::Center;
    name->VAlign = UIVAlign::Middle;
    name->Color = (m_SelectedPath == item.Path) ? EditorStyle::TextBright : EditorStyle::Text;
    name->Text = Ellipsize(item.Name, name->FontSize, s_CardWidth - 8.0f);
}

void ContentDrawer::BuildAssetTile(const Item& InItem) {
    const Item item = InItem;
    const bool placeholder = item.Path.empty();
    const bool editing = IsEditing(item);
    const Vec4 typeColor = EditorIcons::GetAssetColor(item.AssetClass);

    ContentTile* card = m_Grid->Add<ContentTile>();
    card->CornerRadius = 4.0f;
    card->ClipChildren = true;
    card->NormalColor = card->HoverColor = card->PressedColor = Vec4(0.0f);

    if (!placeholder) {
        card->Clicked = [this, item] { m_SelectedPath = item.Path; };
        card->DoubleClicked = [this, item] {
            if (item.AssetPtr) {
                OpenAsset(item.AssetPtr);
            } else {
                AE_WARN("Asset {0} ({1}) is not registered", item.Name, item.Path);
            }
        };
        card->SecondaryClicked = [this, card, item](const Vec2& InCursorPos) {
            m_SelectedPath = item.Path;
            UIMenuModel menu;
            BuildItemMenu(menu, item);
            UIContextMenu::OpenAt(*card, InCursorPos, menu);
        };
        card->DragStarted = [this, item] { BeginDrag(item); };
        card->DragMoved = [this](const Vec2& InCursorPos) { DragOver(InCursorPos); };
        card->DragEnded = [this] { EndDrag(); };
    }

    // The frame is the card's background and its 1px outline in one: the column inside is inset
    // by a pixel, so the frame colour rims the thumbnail and backs the two text rows.
    UIRoundedQuad* frame = card->Add<UIRoundedQuad>();
    frame->Fill();
    frame->CornerRadius = Vec4(4.0f);
    frame->ClipChildren = true;
    frame->Color = s_CardBase;
    if (!placeholder) {
        frame->Bind = [this, card, frame, item] {
            const bool selected = (m_SelectedPath == item.Path);
            frame->Color = selected ? s_SelectedColor : (card->IsHovered() ? s_CardHover : s_CardBase);
        };
    }

    UIVStack* column = frame->Add<UIVStack>();
    column->Fill();
    column->Padding = UIPadding(1.0f, 1.0f);

    UIRoundedQuad* thumb = column->Add<UIRoundedQuad>();
    thumb->Size = { 1.0_rel, 1.0_rel };
    thumb->Color = s_ThumbBackground;
    thumb->CornerRadius = Vec4(3.0f, 3.0f, 0.0f, 0.0f);

    UISvg* icon = thumb->Add<UISvg>();
    icon->Center(Vec2(38.0f, 38.0f));
    icon->Image = EditorIcons::GetAssetIcon(item.AssetClass);
    icon->Tint = typeColor;

    if (item.AssetPtr) {
        if (!m_Thumbnails.Get()) {
            m_Thumbnails = Object::Create<ThumbnailRenderer>();
        }
        if (Texture* thumbnail = m_Thumbnails->GetThumbnail(item.AssetPtr)) {
            UIImage* preview = thumb->Add<UIImage>();
            preview->Center({ 1.0_rel - 8.0_px, 1.0_rel - 8.0_px });
            preview->Image = thumbnail;
            preview->Bind = [preview, icon, thumbnail] {
                const bool ready = thumbnail->GetDefaultView().Get() != nullptr;
                preview->SetEnabled(ready);
                icon->SetEnabled(!ready);
            };
        }
    }

    UIQuad* typeBar = column->Add<UIQuad>();
    typeBar->Size = { 1.0_rel, UIValue(s_TypeBarHeight) };
    typeBar->Color = typeColor;

    UINode* nameRow = column->Add<UINode>();
    nameRow->Size = { 1.0_rel, UIValue(s_NameRowHeight) };
    nameRow->Padding = UIPadding(s_LabelInset, 1.0f);

    if (editing) {
        AddEditField(*nameRow);
    } else {
        UILabel* name = nameRow->Add<UILabel>();
        name->Fill();
        name->FontSize = EditorStyle::FontSize - 1.0f;
        name->VAlign = UIVAlign::Middle;
        name->Color = EditorStyle::TextBright;
        name->Text = Ellipsize(item.Name, name->FontSize, s_CardWidth - 2.0f * s_LabelInset - 2.0f);
    }

    UILabel* type = column->Add<UILabel>();
    type->Size = { 1.0_rel, UIValue(s_TypeRowHeight) };
    type->Padding = UIPadding(s_LabelInset, 0.0f);
    type->FontSize = EditorStyle::FontSize - 3.0f;
    type->VAlign = UIVAlign::Middle;
    type->Color = EditorStyle::TextDim;
    const String typeName = item.AssetClass.Name.empty() ? String("Asset") : item.AssetClass.GetDisplayName();
    type->Text = Ellipsize(typeName, type->FontSize, s_CardWidth - 2.0f * s_LabelInset - 2.0f);
}

/* -------------------------------- Creating & renaming -------------------------------- */

void ContentDrawer::BeginCreateFolder() {
    const String dir = DirFor(m_Mount, m_RelPath);

    m_Edit = InlineEdit();
    m_Edit.Active = true;
    m_Edit.IsFolder = true;
    m_Edit.Name = MakeUniqueName(dir, "NewFolder", true);
    m_Edit.Commit = [this, dir](const String& InName) {
        const String path = dir + "/" + InName;
        if (fs::exists(path)) {
            AE_WARN("'{0}' already exists", path);
            return;
        }
        std::error_code error;
        fs::create_directories(path, error);
        if (error) {
            AE_ERROR("Failed to create '{0}': {1}", path, error.message());
            return;
        }
        m_SelectedPath = path;
    };
    m_NavDirty = true;
}

void ContentDrawer::BeginCreateAsset(const Class& InAssetClass, const String& InBaseName,
                                     std::function<Asset*(const String&, const String&)> InFactory) {
    const String dir = DirFor(m_Mount, m_RelPath);

    m_Edit = InlineEdit();
    m_Edit.Active = true;
    m_Edit.AssetClass = InAssetClass;
    m_Edit.Name = MakeUniqueName(dir, InBaseName, false);
    m_Edit.Commit = [this, dir, InFactory](const String& InName) {
        const String path = dir + "/" + InName + ".asset";
        if (fs::exists(path)) {
            AE_WARN("'{0}' already exists", path);
            return;
        }
        if (Asset* asset = InFactory(dir, InName)) {
            m_SelectedPath = AssetManager::Get().GetAssetPath(asset->GetId());
        }
    };
    m_NavDirty = true;
}

void ContentDrawer::BeginRename(const Item& InItem) {
    const Item item = InItem;

    m_Edit = InlineEdit();
    m_Edit.Active = true;
    m_Edit.IsFolder = item.IsFolder;
    m_Edit.Path = item.Path;
    m_Edit.Name = item.Name;
    m_Edit.AssetClass = item.AssetClass;
    m_Edit.Commit = [this, item](const String& InName) { RenameItem(item, InName); };
    m_NavDirty = true;
}

void ContentDrawer::CommitEdit(const String& InName) {
    if (!m_Edit.Active) {
        return;
    }
    // Taken before the commit runs, so tearing the field down mid-commit cannot re-enter.
    const InlineEdit edit = m_Edit;
    m_Edit = InlineEdit();

    const String name = SanitizeName(InName);
    if (!name.empty() && edit.Commit) {
        edit.Commit(name);
    }
    m_NavDirty = true;
}

void ContentDrawer::CancelEdit() {
    if (!m_Edit.Active) {
        return;
    }
    m_Edit = InlineEdit();
    m_NavDirty = true;
}

void ContentDrawer::OpenNewBlueprintDialog() {
    if (!m_Grid) {
        return;
    }
    UIModalDialog* dialog = UIModalDialog::Open(*m_Grid, "New Blueprint", Vec2(430.0f, 172.0f));
    if (!dialog) {
        return;
    }

    const String dir = DirFor(m_Mount, m_RelPath);

    struct Draft {
        Class ParentClass = Node3D::StaticClass();
        String Name;
    };
    std::shared_ptr<Draft> draft = std::make_shared<Draft>();
    draft->Name = MakeUniqueName(dir, "NewBlueprint", false);

    UINode* classField = dialog->AddField("Parent Class", 26.0f);
    UIButton* classButton = classField->Add<UIButton>();
    classButton->Fill();
    classButton->SetCaption(draft->ParentClass.GetDisplayName());
    EditorStyle::ApplyButtonStyle(*classButton);
    classButton->Bind = [classButton, draft] { classButton->SetCaption(draft->ParentClass.GetDisplayName()); };
    classButton->Clicked = [classButton, draft] {
        UIMenuModel menu;
        menu.Searchable("Search classes");
        menu.MinWidth(260.0f);

        Array<Class> classes = Class::GetSubclassesOf(Node::StaticClass());
        classes.Sort([](const Class& InA, const Class& InB) { return InA.Name < InB.Name; });
        for (const Class& nodeClass : classes) {
            // Reflected classes without a default constructor cannot be spawned, and probing is
            // the only way to tell them apart.
            Object* probe = Object::Create(nodeClass);
            if (!probe) {
                continue;
            }
            delete probe;
            menu.Item(nodeClass.Name, [draft, nodeClass] { draft->ParentClass = nodeClass; })
                .Icon(EditorIcons::GetNodeIcon(nodeClass))
                .Checked(draft->ParentClass == nodeClass);
        }
        UIContextMenu::OpenUnder(*classButton, menu);
    };

    UINode* nameField = dialog->AddField("Name", 24.0f);
    UITextArea* nameInput = nameField->Add<UITextArea>();
    nameInput->Fill();
    nameInput->Padding = UIPadding(5.0f, 0.0f);
    nameInput->SingleLine = true;
    nameInput->FontSize = EditorStyle::FontSize;
    nameInput->TextColor = EditorStyle::TextBright;
    nameInput->CaretColor = EditorStyle::TextBright;
    nameInput->BackgroundColor = EditorStyle::PanelDark;
    nameInput->FocusedBorderColor = EditorStyle::Accent;
    nameInput->CornerRadius = 3.0f;
    nameInput->Text = draft->Name;
    nameInput->TextChanged = [draft](const String& InText) { draft->Name = InText; };
    nameInput->SelectAll();
    nameInput->RequestFocus();

    const auto create = [this, dialog, draft, dir] {
        const String name = SanitizeName(draft->Name);
        if (name.empty()) {
            AE_WARN("A Blueprint needs a name");
            return;
        }
        const String path = dir + "/" + name + ".asset";
        if (fs::exists(path)) {
            AE_WARN("'{0}' already exists", path);
            return;
        }

        Blueprint* blueprint = Cast<Blueprint>(AssetManager::Get().CreateAsset(Blueprint::StaticClass(), dir, name));
        if (!blueprint) {
            return;
        }
        NodeRecord* record = new NodeRecord();
        record->ClassName = draft->ParentClass.Name;
        blueprint->SetRoot(SharedObjectPtr<NodeRecord>(record));
        AssetManager::Get().SaveAsset(blueprint);

        m_SelectedPath = path;
        m_NavDirty = true;
        dialog->Close();
        OpenAsset(blueprint);
    };
    nameInput->Submitted = [create](const String&) { create(); };

    dialog->AddButton("Cancel", false, [dialog] { dialog->Close(); });
    dialog->AddButton("Create", true, create);
}

/* -------------------------------- File operations -------------------------------- */

bool ContentDrawer::RenameItem(const Item& InItem, const String& InNewName) {
    if (InNewName == InItem.Name) {
        return true;
    }

    const String parent = fs::path(InItem.Path).parent_path().string();
    const String destination = parent + "/" + InNewName + (InItem.IsFolder ? "" : ".asset");
    if (fs::exists(destination)) {
        AE_WARN("'{0}' already exists", destination);
        return false;
    }

    if (!InItem.IsFolder && InItem.AssetPtr) {
        if (!AssetManager::Get().MoveAsset(InItem.AssetPtr->GetId(), destination)) {
            return false;
        }
    } else {
        std::error_code error;
        fs::rename(InItem.Path, destination, error);
        if (error) {
            AE_ERROR("Failed to rename '{0}': {1}", InItem.Path, error.message());
            return false;
        }
        if (InItem.IsFolder) {
            AssetManager::Get().RebindAssetPaths(InItem.Path, destination);
            PruneLocations();
        }
    }

    if (m_SelectedPath == InItem.Path) {
        m_SelectedPath = destination;
    }
    return true;
}

bool ContentDrawer::MoveItem(const Item& InItem, const String& InMount, const String& InRel) {
    const String targetDir = DirFor(InMount, InRel);
    const String destination = targetDir + "/" + fs::path(InItem.Path).filename().string();
    if (destination == InItem.Path) {
        return true;
    }
    if (InItem.IsFolder && (targetDir == InItem.Path || targetDir.compare(0, InItem.Path.size() + 1, InItem.Path + "/") == 0)) {
        AE_WARN("Cannot move '{0}' into itself", InItem.Name);
        return false;
    }
    if (fs::exists(destination)) {
        AE_WARN("'{0}' already exists", destination);
        return false;
    }

    if (!InItem.IsFolder && InItem.AssetPtr) {
        if (!AssetManager::Get().MoveAsset(InItem.AssetPtr->GetId(), destination)) {
            return false;
        }
    } else {
        std::error_code error;
        fs::create_directories(targetDir, error);
        fs::rename(InItem.Path, destination, error);
        if (error) {
            AE_ERROR("Failed to move '{0}' to '{1}': {2}", InItem.Path, destination, error.message());
            return false;
        }
        if (InItem.IsFolder) {
            AssetManager::Get().RebindAssetPaths(InItem.Path, destination);
            PruneLocations();
        }
    }

    if (m_SelectedPath == InItem.Path) {
        m_SelectedPath.clear();
    }
    m_NavDirty = true;
    return true;
}

bool ContentDrawer::DeleteItem(const Item& InItem) {
    if (!InItem.IsFolder) {
        std::error_code removeError;
        const bool removed = InItem.AssetPtr ? AssetManager::Get().DeleteAsset(InItem.AssetPtr)
                                             : fs::remove(InItem.Path, removeError);
        if (removed && m_SelectedPath == InItem.Path) {
            m_SelectedPath.clear();
        }
        m_NavDirty = true;
        return removed;
    }

    std::error_code error;
    fs::remove_all(InItem.Path, error);
    if (error) {
        AE_ERROR("Failed to delete '{0}': {1}", InItem.Path, error.message());
        return false;
    }
    AssetManager::Get().UnregisterAssetsUnder(InItem.Path);
    if (m_SelectedPath == InItem.Path) {
        m_SelectedPath.clear();
    }
    PruneLocations();
    m_NavDirty = true;
    return true;
}

void ContentDrawer::PruneLocations() {
    for (int32_t i = m_Expanded.Size() - 1; i >= 0; i--) {
        const Array<String> segments = SplitPath(m_Expanded[i]);
        String rel;
        for (int32_t j = 1; j < segments.Size(); j++) {
            rel += (j == 1 ? "" : "/") + segments[j];
        }
        std::error_code ec;
        if (segments.IsEmpty() || !fs::is_directory(DirFor(segments[0], rel), ec)) {
            m_Expanded.RemoveAt(i);
        }
    }

    std::error_code ec;
    if (!fs::is_directory(DirFor(m_Mount, m_RelPath), ec)) {
        NavigateTo(m_Mount, "");
    }
}

/* -------------------------------- Drag & drop -------------------------------- */

void ContentDrawer::RegisterDropTarget(UINode& InNode, const String& InMount, const String& InRel) {
    DropTarget target;
    target.Node = &InNode;
    target.Mount = InMount;
    target.Rel = InRel;
    m_DropTargets.Add(target);
}

void ContentDrawer::BeginDrag(const Item& InItem) {
    m_DragItem = InItem;
    m_Dragging = true;
    m_DropNode = nullptr;

    // An asset card drags two ways at once: onto a folder here, or out into the rest of the
    // editor. Whichever the cursor ends over wins in EndDrag.
    if (!InItem.IsFolder && InItem.AssetPtr && m_Grid) {
        Texture* thumbnail = m_Thumbnails.Get() ? m_Thumbnails->GetThumbnail(InItem.AssetPtr) : nullptr;
        EditorDragDrop::Begin(*m_Grid, InItem.AssetPtr, thumbnail, InItem.Name);
    }
}

void ContentDrawer::DragOver(const Vec2& InCursorPos) {
    if (!m_Dragging) {
        return;
    }
    EditorDragDrop::Update(InCursorPos);
    m_DropNode = nullptr;
    for (const DropTarget& target : m_DropTargets) {
        UINode* node = target.Node.Get();
        if (!node || !node->IsEnabled() || !node->HitTest(InCursorPos)) {
            continue;
        }
        // Dropping where the item already sits, or into itself, is not a move.
        if (DirFor(target.Mount, target.Rel) == fs::path(m_DragItem.Path).parent_path().string()) {
            continue;
        }
        if (m_DragItem.IsFolder && DirFor(target.Mount, target.Rel).compare(0, m_DragItem.Path.size(), m_DragItem.Path) == 0) {
            continue;
        }
        m_DropNode = node;
        m_DropLocation = { target.Mount, target.Rel };
        break;
    }
}

void ContentDrawer::EndDrag() {
    if (m_Dragging && m_DropNode.Get()) {
        EditorDragDrop::Cancel();
        MoveItem(m_DragItem, m_DropLocation.Mount, m_DropLocation.Rel);
    } else {
        EditorDragDrop::Drop();
    }
    m_Dragging = false;
    m_DropNode = nullptr;
}
