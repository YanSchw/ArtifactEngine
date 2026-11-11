#pragma once
#include "Object.h"
#include "Common/String.h"
#include "Common/Array.h"
#include <memory>

#define FUNCTION(...)

struct Property;

struct Function {
    using NewParamsFn = void*(*)();
    using DeleteParamsFn = void(*)(void*);
    using CopyParamsFn = void(*)(void*, const void*);
    using InvokeFn = void(*)(Object*, void*);

    String Name;
    String OwnerClass;
    String Comment;
    String Category;
    bool IsStatic = false;
    bool IsPure = false;
    Array<Property*> Inputs;
    Array<Property*> Outputs;
    NewParamsFn NewParams = nullptr;
    DeleteParamsFn DeleteParams = nullptr;
    CopyParamsFn CopyParams = nullptr;
    InvokeFn Invoke = nullptr;

    Function(const String& InName, const String& InOwnerClass) : Name(InName), OwnerClass(InOwnerClass) {}

    std::shared_ptr<void> MakeParams() const { return std::shared_ptr<void>(NewParams(), DeleteParams); }
    Property* FindInput(const String& InName) const;
    Property* FindOutput(const String& InName) const;
    bool CanCallOn(const Object* InSelf) const;
    bool Call(Object* InSelf, void* InParams) const;

    static void RegisterTypeFunctions(const String& InTypename, const Array<Function*>& InFunctions);
    static Array<Function*> GetTypeFunctions(const String& InTypename);
    static Array<Function*> GetAllTypeFunctions(const Class& InClass);
    static Array<Function*> GetAllFunctions();
    static Function* FindFunction(const Class& InClass, const String& InName);
};

class FunctionFrame {
public:
    explicit FunctionFrame(const Function& InFunction) : m_Function(InFunction), m_Params(InFunction.MakeParams()) {}

    template<typename T>
    T& Get(const String& InName) { return *(T*)GetValuePtr(InName); }

    void* GetValuePtr(const String& InName) const;

    bool Invoke(Object* InSelf = nullptr) { return m_Function.Call(InSelf, m_Params.get()); }
    void* GetParams() const { return m_Params.get(); }

private:
    const Function& m_Function;
    std::shared_ptr<void> m_Params;
};
