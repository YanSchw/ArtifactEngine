#include "ProjectSettingsDetailsCustomization.h"
#include "DetailsCategory.h"
#include "DetailsRow.h"
#include "Tabs/DetailsTab.h"
#include "Core/EngineConfig.h"
#include "Core/EngineSettings.h"
#include "UI/EditorStyle.h"
#include "UI/EditorIcons.h"
#include "UI/UITooltip.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UISvg.h"
#include <cctype>

static String SettingsLabel(const ProjectSettings& InSettings, const String& InName) {
    const String section = InSettings.GetSection();
    if (InName.size() > section.size() && InName.starts_with(section) && std::isupper((unsigned char)InName[section.size()])) {
        return DetailsCustomization::PrettyPropertyName(InName.substr(section.size()));
    }
    return DetailsCustomization::PrettyPropertyName(InName);
}

static Array<ProjectSettings*> GetSortedSettings() {
    Array<ProjectSettings*> settings;
    for (const SharedObjectPtr<ProjectSettings>& entry : ProjectConfig::Get().GetSettings()) {
        settings.Add(entry.Get());
    }
    settings.Sort([](ProjectSettings* const& InA, ProjectSettings* const& InB) {
        const bool packagingA = InA->IsA<PackagingSettings>();
        const bool packagingB = InB->IsA<PackagingSettings>();
        return packagingA != packagingB ? packagingB : InA->GetSection() < InB->GetSection();
    });
    return settings;
}

float ProjectSettingsDetailsCustomization::BuildHeader(UINode& InHeader, Object* InObject, DetailsTab& InTab) {
    (void)InObject;
    (void)InTab;
    UILabel* title = InHeader.Add<UILabel>();
    title->Fill();
    title->Position = Vec2(10.0f, 0.0f);
    title->FontSize = EditorStyle::FontSize + 1.0f;
    title->Color = EditorStyle::TextBright;
    title->VAlign = UIVAlign::Middle;
    title->Text = "Project Settings";

    UILabel* path = InHeader.Add<UILabel>();
    path->Fill();
    path->Padding = UIPadding(0.0f, 0.0f, 12.0f, 0.0f);
    path->FontSize = EditorStyle::FontSize;
    path->Color = EditorStyle::TextDim;
    path->HAlign = UIHAlign::Right;
    path->VAlign = UIVAlign::Middle;
    path->Text = EngineConfig::GetConfigDir();
    return 32.0f;
}

void ProjectSettingsDetailsCustomization::BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) {
    (void)InObject;
    for (ProjectSettings* settings : GetSortedSettings()) {
        DetailsCategory& category = AddCategory(InList, InTab, PrettyPropertyName(settings->GetSection()), 0);
        const WeakObjectPtr<Object> weak(settings);
        for (Property* property : settings->GetConfigProperties()) {
            const String label = SettingsLabel(*settings, property->Name);
            if (DetailsRow* row = AddPropertyRow(*category.GetBody(), InTab, weak, 0, property, 0, nullptr, label)) {
                BindSettingsRow(*row, InTab, settings, property);
            }
        }
    }
}

void ProjectSettingsDetailsCustomization::BindSettingsRow(DetailsRow& InRow, DetailsTab& InTab, ProjectSettings* InSettings,
                                                          Property* InProperty) {
    const WeakObjectPtr<ProjectSettings> settings(InSettings);
    const WeakObjectPtr<DetailsTab> tab(&InTab);
    InRow.IsOverridden = [settings, InProperty] {
        return settings.Get() && ProjectConfig::Get().IsCustomized(settings.Get(), InProperty);
    };
    InRow.ResetAction = [settings, InProperty, tab] {
        ProjectSettings* target = settings.Get();
        const ProjectSettings* defaults = target ? ProjectSettings::GetDefault(target->GetClass()) : nullptr;
        if (!defaults || !tab.Get()) {
            return;
        }
        tab.Get()->RecordEdit("Reset " + PrettyPropertyName(InProperty->Name), target);
        InProperty->CopyValue(target, defaults);
        NotifyPropertyEdited(tab.Get(), target, InProperty->Name);
    };
    AddFreezeButton(InRow, InTab, InProperty->Name);
}

void ProjectSettingsDetailsCustomization::AddFreezeButton(DetailsRow& InRow, DetailsTab& InTab, const String& InName) {
    UIButton* button = InRow.AddTrailingHost(22.0f)->Add<UIButton>();
    button->Anchor = button->Pivot = Vec2(0.5f);
    button->Size = Vec2(20.0f, 20.0f);
    button->NormalColor = Vec4(0.0f);
    button->HoverColor = EditorStyle::TabHover;
    button->PressedColor = EditorStyle::ButtonPressed;

    UISvg* icon = button->Add<UISvg>();
    icon->Center(Vec2(13.0f, 13.0f));
    UITooltip* tooltip = button->Add<UITooltip>();
    button->Bind = [icon, tooltip, InName] {
        const bool frozen = ProjectConfig::Get().IsFrozen(InName);
        icon->Image = frozen ? EditorIcons::Lock() : EditorIcons::LockOpen();
        icon->Tint = frozen ? EditorStyle::AccentBright : Vec4(Vec3(EditorStyle::TextDim), 0.5f);
        tooltip->Text = ProjectConfig::IsAlwaysFrozen(InName) ? "Always frozen: packaged builds bake this value into the binary"
                      : frozen ? "Frozen: packaged builds bake this value into the binary" : "Freeze for packaging";
    };

    const WeakObjectPtr<DetailsTab> tab(&InTab);
    button->Clicked = [tab, InName] {
        ProjectConfig& config = ProjectConfig::Get();
        PackagingSettings* packaging = config.GetPackagingSettings();
        if (!packaging || !tab.Get() || ProjectConfig::IsAlwaysFrozen(InName)) {
            return;
        }
        const bool frozen = config.IsFrozen(InName);
        tab.Get()->RecordEdit((frozen ? "Unfreeze " : "Freeze ") + PrettyPropertyName(InName), packaging);
        config.SetFrozen(InName, !frozen);
        NotifyPropertyEdited(tab.Get(), packaging, "FrozenConfigVars");
        tab.Get()->MarkDirty();
    };
}
