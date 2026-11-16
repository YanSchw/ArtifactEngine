#pragma once
#include "Object/Object.h"
#include "Common/Array.h"
#include "ProjectSettings.gen.h"

class ProjectSettings : public Object {
public:
    ARTIFACT_CLASS();

    virtual String GetConfigFile() const { return "Game"; }
    virtual String GetSection() const;

    static Array<Class> GetSettingsClasses();
    static ProjectSettings* Get(const Class& InClass);
    template<typename T>
    static T* Get() { return Cast<T>(Get(T::StaticClass())); }
    static const ProjectSettings* GetDefault(const Class& InClass);

    Array<Property*> GetConfigProperties() const;
    String GetConfigValue(Property* InProperty) const;
    bool SetConfigValue(Property* InProperty, const String& InValue);

protected:
    ProjectSettings() = default;

private:
    static void RegisterDefaults();
    static void OnConfigVarChanged(const String& InName, const String& InValue);

    friend class EngineConfig;
};
