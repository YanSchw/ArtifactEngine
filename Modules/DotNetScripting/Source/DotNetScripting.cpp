#include "DotNetScripting.h"
#include "DotNetHost.h"
#include "ScriptClass.h"
#include "Core/EngineConfig.h"
#include "Core/Log.h"
#include "Object/Function.h"
#include "Object/Property.h"
#include "Serialization/ThirdParty/nlohmann/json.hpp"
#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <memory>
#include <set>

struct ScriptFieldDesc {
    const char* Name;
    int32_t Kind;
    uint32_t Offset;
};

struct ScriptClassDesc {
    const char* Name;
    const char* Parent;
    const char* Module;
    uint8_t Ticks;
    ScriptFieldDesc* Fields;
    int32_t FieldCount;
};

struct NativeApi {
    void* (*FindFunction)(const char*, const char*);
    uint8_t (*CallFunction)(Function*, Object*, void**, void*);
    Object* (*CreateObject)(const char*);
    void (*Retain)(Object*);
    void (*Release)(Object*);
    WeakObjectPtr<Object>* (*NewWeak)(Object*);
    uint8_t (*IsAlive)(WeakObjectPtr<Object>*);
    void (*DeleteWeak)(WeakObjectPtr<Object>*);
    const char* (*ClassNameOf)(Object*);
    const char* (*ParentClassNameOf)(const char*);
    uint8_t (*RegisterScriptClass)(ScriptClassDesc*);
    void (*SetString)(String*, const char*);
    const char* (*GetString)(String*);
    void (*Log)(int32_t, const char*);
};

struct ManagedApi {
    int32_t (*Initialize)(NativeApi*) = nullptr;
    int32_t (*Load)(const char**, int32_t) = nullptr;
    void (*Unload)() = nullptr;
    void (*Restore)() = nullptr;
    void (*CreateInstance)(const char*, Object*, char*) = nullptr;
    void (*Call)(Object*, int32_t, float) = nullptr;
    void (*Destroyed)(Object*) = nullptr;
};

static std::unique_ptr<DotNetHost> s_Host;
static ManagedApi s_Managed;

void ScriptBridge::CreateManaged(Object& InObject, const ScriptClassInfo& InInfo, char* InStorage) {
    if (s_Managed.CreateInstance) {
        s_Managed.CreateInstance(InInfo.Name.c_str(), &InObject, InStorage);
    }
}

void ScriptBridge::DestroyManaged(Object& InObject) {
    if (s_Managed.Destroyed) {
        s_Managed.Destroyed(&InObject);
    }
}

void ScriptBridge::Call(Object& InObject, ScriptCall InCall, float InDeltaTime) {
    if (s_Managed.Call) {
        s_Managed.Call(&InObject, (int32_t)InCall, InDeltaTime);
    }
}

static uint8_t RegisterScriptClass(ScriptClassDesc* InDesc) {
    Array<ScriptRegistry::FieldDesc> fields;
    for (int32_t i = 0; i < InDesc->FieldCount; i++) {
        fields.Add({ InDesc->Fields[i].Name, (ScriptFieldKind)InDesc->Fields[i].Kind });
    }
    const ScriptClassInfo* info = ScriptRegistry::Register(InDesc->Name, InDesc->Parent, InDesc->Module, InDesc->Ticks != 0, fields);
    if (!info) {
        return 0;
    }
    const int32_t first = (int32_t)info->Fields.Size() - InDesc->FieldCount;
    for (int32_t i = 0; i < InDesc->FieldCount; i++) {
        InDesc->Fields[i].Offset = info->Fields[first + i].Offset;
    }
    return 1;
}

static uint32_t SizeOf(Property& InProperty) {
    if (IntProperty* integer = Cast<IntProperty>(&InProperty)) {
        return integer->NumBits / 8;
    }
    if (FloatProperty* number = Cast<FloatProperty>(&InProperty)) {
        return number->IsDouble ? sizeof(double) : sizeof(float);
    }
    if (EnumProperty* enumeration = Cast<EnumProperty>(&InProperty)) {
        return enumeration->ByteSize;
    }
    if (StructProperty* structure = Cast<StructProperty>(&InProperty)) {
        const String& name = structure->InnerStructTypename;
        return name == "Vec2" ? sizeof(Vec2) : name == "Vec3" ? sizeof(Vec3) : name == "Quat" ? sizeof(Quat) : sizeof(Vec4);
    }
    return 0;
}

static void ReadArgument(Property& InProperty, void* OutValue, void* InArgument) {
    if (Cast<StringProperty>(&InProperty)) {
        const char* text = *(const char**)InArgument;
        *(String*)OutValue = text ? text : "";
    } else if (Cast<WeakObjectPtrProperty>(&InProperty) || Cast<SharedObjectPtrProperty>(&InProperty)) {
        *(WeakObjectPtr<Object>*)OutValue = *(Object**)InArgument;
    } else if (Cast<BoolProperty>(&InProperty)) {
        *(bool*)OutValue = *(uint8_t*)InArgument != 0;
    } else {
        std::memcpy(OutValue, InArgument, SizeOf(InProperty));
    }
}

static void WriteResult(Property& InProperty, void* InValue, void* OutResult, Array<String>& OutStrings) {
    if (Cast<StringProperty>(&InProperty)) {
        OutStrings.Add(*(String*)InValue);
        *(const char**)OutResult = OutStrings.LastItem().c_str();
    } else if (Cast<WeakObjectPtrProperty>(&InProperty) || Cast<SharedObjectPtrProperty>(&InProperty)) {
        *(Object**)OutResult = ((WeakObjectPtr<Object>*)InValue)->Get();
    } else if (Cast<BoolProperty>(&InProperty)) {
        *(uint8_t*)OutResult = *(bool*)InValue ? 1 : 0;
    } else {
        std::memcpy(OutResult, InValue, SizeOf(InProperty));
    }
}

static uint8_t CallFunction(Function* InFunction, Object* InSelf, void** InArguments, void* OutResult) {
    if (!InFunction || !InFunction->CanCallOn(InSelf)) {
        return 0;
    }
    thread_local Array<String> s_Strings;
    s_Strings.Clear();
    s_Strings.GetData().reserve(InFunction->Outputs.Size());

    std::shared_ptr<void> params = InFunction->MakeParams();
    int32_t argument = 0;
    for (Property* input : InFunction->Inputs) {
        ReadArgument(*input, input->GetValuePtr(params.get()), InArguments[argument++]);
    }
    InFunction->Invoke(InSelf, params.get());
    for (Property* output : InFunction->Outputs) {
        const bool isReturn = output->Name == "ReturnValue";
        void* destination = isReturn ? OutResult : InArguments[argument++];
        if (destination) {
            WriteResult(*output, output->GetValuePtr(params.get()), destination, s_Strings);
        }
    }
    return 1;
}

static const char* KeepString(const String& InText) {
    thread_local String s_Buffer;
    s_Buffer = InText;
    return s_Buffer.c_str();
}

static NativeApi MakeNativeApi() {
    NativeApi api;
    api.FindFunction = [](const char* InClass, const char* InName) -> void* { return Function::FindFunction(Class(InClass), InName); };
    api.CallFunction = &CallFunction;
    api.CreateObject = [](const char* InClass) -> Object* {
        Object* object = Object::Create(Class(InClass));
        Object::Retain(object);
        return object;
    };
    api.Retain = &Object::Retain;
    api.Release = &Object::Release;
    api.NewWeak = [](Object* InObject) { return new WeakObjectPtr<Object>(InObject); };
    api.IsAlive = [](WeakObjectPtr<Object>* InWeak) -> uint8_t { return InWeak && InWeak->Get() ? 1 : 0; };
    api.DeleteWeak = [](WeakObjectPtr<Object>* InWeak) { delete InWeak; };
    api.ClassNameOf = [](Object* InObject) { return KeepString(InObject ? InObject->GetClass().Name : String()); };
    api.ParentClassNameOf = [](const char* InClass) { return KeepString(Class(InClass).GetParentClass().Name); };
    api.RegisterScriptClass = &RegisterScriptClass;
    api.SetString = [](String* OutString, const char* InText) { *OutString = InText ? InText : ""; };
    api.GetString = [](String* InString) { return InString->c_str(); };
    api.Log = [](int32_t InLevel, const char* InText) {
        if (InLevel >= 3) {
            AE_ERROR("{0}", InText);
        } else if (InLevel == 2) {
            AE_WARN("{0}", InText);
        } else {
            AE_INFO("{0}", InText);
        }
    };
    return api;
}

void DotNetScripting::Initialize() {
    const std::filesystem::path directory = std::filesystem::path(EngineConfig::GetProjectDir()) / "Binaries" / "DotNet";
    std::ifstream file(directory / "Scripting.json");
    if (!file) {
        AE_INFO("C# scripting is off: no scripting modules were built");
        return;
    }
    const nlohmann::json manifest = nlohmann::json::parse(file, nullptr, false);
    if (manifest.is_discarded()) {
        AE_ERROR("C# scripting is off: {0} is not valid JSON", (directory / "Scripting.json").string());
        return;
    }
    for (const nlohmann::json& name : manifest.value("Assemblies", nlohmann::json::array())) {
        m_Assemblies.Add((directory / (name.get<std::string>() + ".dll")).string());
    }
    for (const nlohmann::json& watch : manifest.value("Watch", nlohmann::json::array())) {
        m_WatchDirectories.Add(watch.get<std::string>());
    }
    m_BuildCommand = "\"" + manifest.value("DotNet", std::string("dotnet")) + "\" build \"" + manifest.value("Solution", std::string())
                   + "\" -c Debug --nologo -v q -clp:ErrorsOnly";

    s_Host = std::make_unique<DotNetHost>();
    if (!s_Host->Start((directory / "Artifact.runtimeconfig.json").string()) || !s_Host->LoadAssembly((directory / "Artifact.dll").string())) {
        s_Host.reset();
        return;
    }

    const String host = "Artifact.Host, Artifact";
    auto initialize = (int32_t(*)(NativeApi*))s_Host->GetFunction(host, "Initialize");
    s_Managed.Load = (int32_t(*)(const char**, int32_t))s_Host->GetFunction(host, "Load");
    s_Managed.Unload = (void(*)())s_Host->GetFunction(host, "Unload");
    s_Managed.Restore = (void(*)())s_Host->GetFunction(host, "Restore");
    s_Managed.CreateInstance = (void(*)(const char*, Object*, char*))s_Host->GetFunction(host, "CreateInstance");
    s_Managed.Call = (void(*)(Object*, int32_t, float))s_Host->GetFunction(host, "Call");
    s_Managed.Destroyed = (void(*)(Object*))s_Host->GetFunction(host, "Destroyed");

    static NativeApi s_Api = MakeNativeApi();
    if (!initialize || !s_Managed.Load || !initialize(&s_Api) || !LoadAssemblies()) {
        AE_ERROR("C# scripting failed to initialize");
        s_Managed = ManagedApi();
        return;
    }
    m_SourceStamp = ScanSources();
    m_Ready = true;
    AE_INFO("C# scripting ready ({0} script classes)", ScriptRegistry::GetClassCount());
}

bool DotNetScripting::LoadAssemblies() {
    Array<const char*> paths;
    for (const String& path : m_Assemblies) {
        paths.Add(path.c_str());
    }
    return s_Managed.Load(paths.GetData().data(), (int32_t)paths.Size()) != 0;
}

void DotNetScripting::Reload() {
    const auto start = std::chrono::steady_clock::now();
    s_Managed.Unload();
    ScriptRegistry::Clear();
    const bool loaded = LoadAssemblies();
    const ScriptRegistry::MigrationResult result = ScriptRegistry::Migrate();
    s_Managed.Restore();
    m_ReloadCount++;

    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - start).count();
    if (!loaded) {
        AE_ERROR("C# hot reload could not load the rebuilt assemblies");
    }
    AE_INFO("C# hot reload: {0} script classes, {1} live objects kept in {2} ms", ScriptRegistry::GetClassCount(), result.Migrated, (int64_t)milliseconds);
    if (result.Stale > 0) {
        AE_WARN("{0} live script objects no longer match their class and fell back to their native base; reopen their scene to recreate them", result.Stale);
    }
}

void DotNetScripting::Tick(double InDeltaTime) {
    if (!m_Ready) {
        return;
    }
    bool succeeded = false;
    String output;
    if (m_Compiler.TakeResult(succeeded, output)) {
        if (succeeded) {
            Reload();
        } else {
            ReportCompileErrors(output);
        }
    }

    m_PollTimer += InDeltaTime;
    if (m_PollTimer < 0.5 || m_Compiler.IsRunning()) {
        return;
    }
    m_PollTimer = 0.0;
    const size_t stamp = ScanSources();
    if (stamp != m_SourceStamp) {
        m_SourceStamp = stamp;
        AE_INFO("C# sources changed, recompiling");
        m_Compiler.Start(m_BuildCommand);
    }
}

size_t DotNetScripting::ScanSources() const {
    size_t stamp = 0;
    std::error_code error;
    for (const String& directory : m_WatchDirectories) {
        for (auto it = std::filesystem::recursive_directory_iterator(directory, error); it != std::filesystem::recursive_directory_iterator(); it.increment(error)) {
            if (it->path().extension() != ".cs" || !it->is_regular_file(error)) {
                continue;
            }
            const size_t time = (size_t)it->last_write_time(error).time_since_epoch().count();
            stamp ^= std::hash<std::string>()(it->path().string()) * 31 + time * 7 + (size_t)it->file_size(error);
        }
    }
    return stamp;
}

void DotNetScripting::ReportCompileErrors(const String& InOutput) const {
    std::set<String> reported;
    size_t start = 0;
    while (start < InOutput.size()) {
        const size_t end = std::min(InOutput.find('\n', start), InOutput.size());
        String line = InOutput.substr(start, end - start);
        const size_t project = line.rfind(" [");
        if (project != String::npos) {
            line = line.substr(0, project);
        }
        if (line.find(": error ") != String::npos && reported.insert(line).second) {
            AE_ERROR("{0}", line);
        }
        start = end + 1;
    }
    if (reported.empty()) {
        AE_ERROR("C# compile failed:\n{0}", InOutput);
    }
    AE_WARN("C# hot reload skipped, the last working scripts keep running");
}
