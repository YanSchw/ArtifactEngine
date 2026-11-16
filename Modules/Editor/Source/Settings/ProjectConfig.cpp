#include "ProjectConfig.h"
#include "Core/EngineConfig.h"
#include "Core/EngineSettings.h"
#include "Core/Log.h"
#include "Platform/FileIO.h"
#include <filesystem>

ProjectConfig& ProjectConfig::Get() {
    static ProjectConfig s_Config;
    return s_Config;
}

void ProjectConfig::Load() {
    LoadFiles();
    if (m_Settings.IsEmpty()) {
        for (const Class& settingsClass : ProjectSettings::GetSettingsClasses()) {
            m_Settings.Add(SharedObjectPtr<ProjectSettings>(Object::Create<ProjectSettings>(settingsClass)));
        }
    }
    for (const SharedObjectPtr<ProjectSettings>& settings : m_Settings) {
        const ProjectSettings* defaults = ProjectSettings::GetDefault(settings->GetClass());
        for (Property* property : settings->GetConfigProperties()) {
            property->CopyValue(settings.Get(), defaults);
            Array<KeyLocation> locations = FindKey(property->Name);
            if (locations.IsEmpty()) {
                continue;
            }
            const ConfigFile& file = m_Files[locations.LastItem().File];
            const String& value = *file.Ini.Find(locations.LastItem().Section, property->Name);
            if (!settings->SetConfigValue(property, value)) {
                AE_WARN("Ignoring invalid {0}={1} in {2}", property->Name, value, file.Path);
            }
        }
    }
}

void ProjectConfig::Save() {
    LoadFiles();
    for (const SharedObjectPtr<ProjectSettings>& settings : m_Settings) {
        SaveSettings(settings.Get());
    }
    Flush();
}

ProjectSettings* ProjectConfig::FindSettings(const Class& InClass) const {
    for (const SharedObjectPtr<ProjectSettings>& settings : m_Settings) {
        if (settings->GetClass() == InClass) {
            return settings.Get();
        }
    }
    return nullptr;
}

PackagingSettings* ProjectConfig::GetPackagingSettings() const {
    return Cast<PackagingSettings>(FindSettings(PackagingSettings::StaticClass()));
}

bool ProjectConfig::IsCustomized(ProjectSettings* InSettings, Property* InProperty) const {
    const ProjectSettings* defaults = ProjectSettings::GetDefault(InSettings->GetClass());
    return defaults && InSettings->GetConfigValue(InProperty) != defaults->GetConfigValue(InProperty);
}

bool ProjectConfig::IsAlwaysFrozen(const String& InName) {
    return InName == "ProjectName";
}

bool ProjectConfig::IsFrozen(const String& InName) const {
    PackagingSettings* packaging = GetPackagingSettings();
    return IsAlwaysFrozen(InName) || (packaging && packaging->FrozenConfigVars.Contains(InName));
}

void ProjectConfig::SetFrozen(const String& InName, bool InFrozen) {
    PackagingSettings* packaging = GetPackagingSettings();
    if (!packaging || IsAlwaysFrozen(InName) || IsFrozen(InName) == InFrozen) {
        return;
    }
    if (InFrozen) {
        packaging->FrozenConfigVars.Add(InName);
    } else {
        packaging->FrozenConfigVars.Remove(InName);
    }
}

void ProjectConfig::LoadFiles() {
    m_Files.Clear();
    std::error_code error;
    for (const std::filesystem::directory_entry& entry : std::filesystem::directory_iterator(EngineConfig::GetConfigDir(), error)) {
        if (entry.is_regular_file(error) && entry.path().extension() == ".ini") {
            ConfigFile file;
            file.Name = entry.path().stem().string();
            file.Path = entry.path().string();
            file.Ini = IniFile::Parse(FileIO::ReadFileToString(file.Path));
            m_Files.Add(file);
        }
    }
    m_Files.Sort([](const ConfigFile& InA, const ConfigFile& InB) { return InA.Path < InB.Path; });
}

Array<ProjectConfig::KeyLocation> ProjectConfig::FindKey(const String& InKey) const {
    Array<KeyLocation> locations;
    for (int32_t i = 0; i < m_Files.Size(); i++) {
        for (const IniFile::Entry& entry : m_Files[i].Ini.GetEntries()) {
            if (entry.Key == InKey) {
                locations.Add(KeyLocation{ i, entry.Section });
            }
        }
    }
    return locations;
}

ProjectConfig::ConfigFile& ProjectConfig::GetFile(const String& InName) {
    for (ConfigFile& file : m_Files) {
        if (file.Name == InName) {
            return file;
        }
    }
    ConfigFile file;
    file.Name = InName;
    file.Path = (std::filesystem::path(EngineConfig::GetConfigDir()) / (InName + ".ini")).string();
    m_Files.Add(file);
    return m_Files.LastItem();
}

void ProjectConfig::SaveSettings(ProjectSettings* InSettings) {
    const ProjectSettings* defaults = ProjectSettings::GetDefault(InSettings->GetClass());
    for (Property* property : InSettings->GetConfigProperties()) {
        const String& name = property->Name;
        const String value = InSettings->GetConfigValue(property);
        const String defaultValue = defaults->GetConfigValue(property);
        Array<KeyLocation> locations = FindKey(name);
        const String previous = locations.IsEmpty() ? defaultValue
                                                    : *m_Files[locations.LastItem().File].Ini.Find(locations.LastItem().Section, name);

        if (value != defaultValue || IsFrozen(name)) {
            if (locations.IsEmpty()) {
                ConfigFile& file = GetFile(InSettings->GetConfigFile());
                file.Ini.Set(InSettings->GetSection(), name, value);
                file.Dirty = true;
            } else if (previous != value) {
                ConfigFile& file = m_Files[locations.LastItem().File];
                file.Ini.Set(locations.LastItem().Section, name, value);
                file.Dirty = true;
            }
        } else {
            for (const KeyLocation& location : locations) {
                m_Files[location.File].Ini.Remove(location.Section, name);
                m_Files[location.File].Dirty = true;
            }
        }
        if (previous != value) {
            EngineConfig::SetConfigVar(name, value);
        }
    }
}

void ProjectConfig::Flush() {
    for (ConfigFile& file : m_Files) {
        if (!file.Dirty) {
            continue;
        }
        file.Dirty = false;
        std::error_code error;
        if (file.Ini.IsEmpty()) {
            std::filesystem::remove(file.Path, error);
            continue;
        }
        std::filesystem::create_directories(std::filesystem::path(file.Path).parent_path(), error);
        if (!FileIO::WriteStringToFile(file.Path, file.Ini.ToString())) {
            AE_WARN("Failed to save the project config to {0}", file.Path);
        }
    }
}
