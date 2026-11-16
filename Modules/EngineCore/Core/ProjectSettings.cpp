#include "ProjectSettings.h"
#include "EngineConfig.h"
#include "Log.h"
#include "Common/Map.h"
#include "Serialization/Json.h"
#include "Serialization/ThirdParty/nlohmann/json.hpp"
#include <cstdio>
#include <cstdlib>

using json = nlohmann::json;

struct ConfigVarOwner {
    Class SettingsClass;
    Property* SettingsProperty = nullptr;
};

static Array<Class>& GetRegisteredClasses() {
    static Array<Class> s_Classes;
    return s_Classes;
}

static Map<String, ProjectSettings*>& GetInstances() {
    static Map<String, ProjectSettings*> s_Instances;
    return s_Instances;
}

static Map<String, ProjectSettings*>& GetDefaults() {
    static Map<String, ProjectSettings*> s_Defaults;
    return s_Defaults;
}

static Map<String, ConfigVarOwner>& GetOwners() {
    static Map<String, ConfigVarOwner> s_Owners;
    return s_Owners;
}

static bool IsTextValue(Property* InProperty) {
    return Cast<StringProperty>(InProperty) || Cast<UUIDProperty>(InProperty) || Cast<EnumProperty>(InProperty)
        || Cast<SubclassOfProperty>(InProperty);
}

template<typename T>
static String FormatShortest(T InValue, T (*InParse)(const char*, char**)) {
    char buffer[64];
    for (int precision = 1; precision <= 17; precision++) {
        std::snprintf(buffer, sizeof(buffer), "%.*g", precision, (double)InValue);
        if (InParse(buffer, nullptr) == InValue) {
            break;
        }
    }
    return buffer;
}

String ProjectSettings::GetSection() const {
    const String name = GetClass().Name;
    const String suffix = "Settings";
    if (name.size() > suffix.size() && name.ends_with(suffix)) {
        return name.substr(0, name.size() - suffix.size());
    }
    return name;
}

Array<Class> ProjectSettings::GetSettingsClasses() {
    return GetRegisteredClasses();
}

ProjectSettings* ProjectSettings::Get(const Class& InClass) {
    ProjectSettings** instance = GetInstances().Find(InClass.Name);
    return instance ? *instance : nullptr;
}

const ProjectSettings* ProjectSettings::GetDefault(const Class& InClass) {
    ProjectSettings** defaults = GetDefaults().Find(InClass.Name);
    return defaults ? *defaults : nullptr;
}

Array<Property*> ProjectSettings::GetConfigProperties() const {
    return Property::GetAllTypeProperties(GetClass()).Filter([](Property* const& InProperty) { return !InProperty->IsTransient(); });
}

String ProjectSettings::GetConfigValue(Property* InProperty) const {
    void* value = InProperty->GetValuePtr(const_cast<ProjectSettings*>(this));
    if (FloatProperty* floatProperty = Cast<FloatProperty>(InProperty)) {
        return floatProperty->IsDouble ? FormatShortest<double>(*(double*)value, std::strtod)
                                       : FormatShortest<float>(*(float*)value, std::strtof);
    }
    const json serialized = JsonSerializer::SerializeProperty(InProperty, value);
    if (IsTextValue(InProperty)) {
        return serialized.is_string() ? serialized.get<String>() : String();
    }
    return serialized.dump();
}

bool ProjectSettings::SetConfigValue(Property* InProperty, const String& InValue) {
    void* value = InProperty->GetValuePtr(this);
    json parsed;
    if (Cast<BoolProperty>(InProperty)) {
        if (InValue != "true" && InValue != "1" && InValue != "false" && InValue != "0") {
            return false;
        }
        parsed = InValue == "true" || InValue == "1";
    } else if (IsTextValue(InProperty)) {
        const bool isNull = InValue.empty() && (Cast<UUIDProperty>(InProperty) || Cast<SubclassOfProperty>(InProperty));
        parsed = isNull ? json(nullptr) : json(InValue);
    } else {
        parsed = json::parse(InValue, nullptr, false);
        if (parsed.is_discarded()) {
            return false;
        }
    }
    try {
        JsonSerializer::DeserializeProperty(InProperty, value, parsed);
    } catch (const json::exception&) {
        return false;
    }
    InProperty->NotifyChanged(this);
    return true;
}

void ProjectSettings::RegisterDefaults() {
    Array<Class> classes = Class::GetSubclassesOf(StaticClass()).Filter([](const Class& InClass) {
        return InClass != StaticClass() && !InClass.IsBlueprint() && InClass.IsDefaultConstructable();
    });
    classes.Sort([](const Class& InA, const Class& InB) { return InA.Name < InB.Name; });

    for (const Class& settingsClass : classes) {
        ProjectSettings* defaults = Object::Create<ProjectSettings>(settingsClass);
        ProjectSettings* instance = Object::Create<ProjectSettings>(settingsClass);
        if (!defaults || !instance) {
            continue;
        }
        GetRegisteredClasses().Add(settingsClass);
        GetDefaults()[settingsClass.Name] = defaults;
        GetInstances()[settingsClass.Name] = instance;
        for (Property* property : defaults->GetConfigProperties()) {
            if (const ConfigVarOwner* owner = GetOwners().Find(property->Name)) {
                AE_WARN("Config var '{0}' of {1} is already declared by {2}", property->Name, settingsClass.Name, owner->SettingsClass.Name);
                continue;
            }
            GetOwners()[property->Name] = ConfigVarOwner{ settingsClass, property };
            EngineConfig::SetConfigVar(property->Name, defaults->GetConfigValue(property));
        }
    }
}

void ProjectSettings::OnConfigVarChanged(const String& InName, const String& InValue) {
    const ConfigVarOwner* owner = GetOwners().Find(InName);
    ProjectSettings* instance = owner ? Get(owner->SettingsClass) : nullptr;
    if (instance && !instance->SetConfigValue(owner->SettingsProperty, InValue)) {
        AE_WARN("Config var '{0}' has no valid {1} value: '{2}'", InName, owner->SettingsClass.Name, InValue);
    }
}
