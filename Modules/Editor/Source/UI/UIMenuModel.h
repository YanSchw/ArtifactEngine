#pragma once
#include "Common/String.h"
#include "Common/Array.h"
#include <functional>

class VectorImage;
class UIMenuModel;

struct UIMenuEntry {
    enum class Kind : uint8_t { Action, Section, Separator };

    Kind Type = Kind::Action;
    String Label;
    String Shortcut;
    String Tooltip;
    VectorImage* Icon = nullptr;
    bool Enabled = true;
    bool Checked = false;
    std::function<void()> Activated;
    /* Filled the moment the row opens, so submenu content is only built when it is shown. */
    std::function<void(UIMenuModel&)> Submenu;
};

/** The content of a menu, handed to UIContextMenu to be shown. Entries are added in order and
 *  refined by chaining; every modifier applies to the entry added last:
 *
 *      UIMenuModel menu;
 *      menu.Section("Actor Options");
 *      menu.Item("Rename", [node] { ... }).Shortcut("F2").Icon(EditorIcons::Node());
 *      menu.Submenu("Add Child", [node](UIMenuModel& InSub) {
 *          InSub.Searchable();
 *          InSub.Item("Node3D", [node] { node->CreateChild(Node3D::StaticClass()); });
 *      });
 *      menu.Separator();
 *      menu.Item("Delete", [node] { node->Destroy(); }).Enabled(node->GetParent() != nullptr);
 */
class UIMenuModel {
public:
    UIMenuModel& Item(const String& InLabel, std::function<void()> InActivated = nullptr) {
        UIMenuEntry entry;
        entry.Label = InLabel;
        entry.Activated = std::move(InActivated);
        m_Entries.Add(std::move(entry));
        return *this;
    }

    UIMenuModel& Submenu(const String& InLabel, std::function<void(UIMenuModel&)> InBuild) {
        UIMenuEntry entry;
        entry.Label = InLabel;
        entry.Submenu = std::move(InBuild);
        m_Entries.Add(std::move(entry));
        return *this;
    }

    UIMenuModel& Section(const String& InTitle) {
        UIMenuEntry entry;
        entry.Type = UIMenuEntry::Kind::Section;
        entry.Label = InTitle;
        m_Entries.Add(std::move(entry));
        return *this;
    }

    UIMenuModel& Separator() {
        UIMenuEntry entry;
        entry.Type = UIMenuEntry::Kind::Separator;
        m_Entries.Add(std::move(entry));
        return *this;
    }

    UIMenuModel& Icon(VectorImage* InIcon) { if (UIMenuEntry* e = Last()) e->Icon = InIcon; return *this; }
    UIMenuModel& Shortcut(const String& InText) { if (UIMenuEntry* e = Last()) e->Shortcut = InText; return *this; }
    UIMenuModel& Tooltip(const String& InText) { if (UIMenuEntry* e = Last()) e->Tooltip = InText; return *this; }
    UIMenuModel& Enabled(bool InEnabled) { if (UIMenuEntry* e = Last()) e->Enabled = InEnabled; return *this; }
    UIMenuModel& Checked(bool InChecked) { if (UIMenuEntry* e = Last()) e->Checked = InChecked; return *this; }

    /** Puts a filter field above the entries, matching labels as you type. */
    UIMenuModel& Searchable(const String& InPlaceholder = "Type to search") {
        m_Searchable = true;
        m_SearchPlaceholder = InPlaceholder;
        return *this;
    }
    UIMenuModel& MinWidth(float InPixels) { m_MinWidth = InPixels; return *this; }

    const Array<UIMenuEntry>& GetEntries() const { return m_Entries; }
    bool IsSearchable() const { return m_Searchable; }
    const String& GetSearchPlaceholder() const { return m_SearchPlaceholder; }
    float GetMinWidth() const { return m_MinWidth; }

    bool IsEmpty() const {
        for (const UIMenuEntry& entry : m_Entries) {
            if (entry.Type == UIMenuEntry::Kind::Action) {
                return false;
            }
        }
        return true;
    }

private:
    UIMenuEntry* Last() { return m_Entries.IsEmpty() ? nullptr : &m_Entries[m_Entries.Size() - 1]; }

    Array<UIMenuEntry> m_Entries;
    bool m_Searchable = false;
    String m_SearchPlaceholder;
    float m_MinWidth = 0.0f;
};
