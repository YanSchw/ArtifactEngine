#pragma once
#include "Object/Object.h"
#include "Object/Pointer.h"
#include "Common/Array.h"
#include "Serialization/Ini.h"
#include "ProjectConfig.gen.h"

class ProjectSettings;
class PackagingSettings;

class ProjectConfig : public Object {
public:
    ARTIFACT_CLASS();

    static ProjectConfig& Get();

    void Load();
    void Save();

    const Array<SharedObjectPtr<ProjectSettings>>& GetSettings() const { return m_Settings; }
    ProjectSettings* FindSettings(const Class& InClass) const;
    PackagingSettings* GetPackagingSettings() const;

    bool IsCustomized(ProjectSettings* InSettings, Property* InProperty) const;
    static bool IsAlwaysFrozen(const String& InName);
    bool IsFrozen(const String& InName) const;
    void SetFrozen(const String& InName, bool InFrozen);

private:
    struct ConfigFile {
        String Name;
        String Path;
        IniFile Ini;
        bool Dirty = false;
    };
    struct KeyLocation {
        int32_t File = -1;
        String Section;
    };

    void LoadFiles();
    Array<KeyLocation> FindKey(const String& InKey) const;
    ConfigFile& GetFile(const String& InName);
    void SaveSettings(ProjectSettings* InSettings);
    void Flush();

    Array<ConfigFile> m_Files;
    Array<SharedObjectPtr<ProjectSettings>> m_Settings;
};
