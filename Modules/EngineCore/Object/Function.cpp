#include "Function.h"
#include "Property.h"
#include "Common/Map.h"
#include "Core/Log.h"

static Map<String, Array<Function*>>& GetTypeFunctionMap() {
    static Map<String, Array<Function*>> map;
    return map;
}

static Property* FindNamed(const Array<Property*>& InProperties, const String& InName) {
    for (Property* property : InProperties) {
        if (property->Name == InName) {
            return property;
        }
    }
    return nullptr;
}

Property* Function::FindInput(const String& InName) const {
    return FindNamed(Inputs, InName);
}

Property* Function::FindOutput(const String& InName) const {
    return FindNamed(Outputs, InName);
}

bool Function::CanCallOn(const Object* InSelf) const {
    return IsStatic || (InSelf && InSelf->IsA(Class(OwnerClass)));
}

bool Function::Call(Object* InSelf, void* InParams) const {
    if (!Invoke || !CanCallOn(InSelf)) {
        AE_WARN("'{0}::{1}' needs a target of class '{0}'", OwnerClass, Name);
        return false;
    }
    Invoke(InSelf, InParams);
    return true;
}

void Function::RegisterTypeFunctions(const String& InTypename, const Array<Function*>& InFunctions) {
    GetTypeFunctionMap()[InTypename] += InFunctions;
}

Array<Function*> Function::GetTypeFunctions(const String& InTypename) {
    const Array<Function*>* functions = GetTypeFunctionMap().Find(InTypename);
    return functions ? *functions : Array<Function*>();
}

Array<Function*> Function::GetAllTypeFunctions(const Class& InClass) {
    Array<Function*> functions;
    for (Class current = InClass; current != Class::None; current = current.GetParentClass()) {
        functions += GetTypeFunctions(current.Name);
        if (current.Name == "Object") {
            break;
        }
    }
    return functions;
}

Array<Function*> Function::GetAllFunctions() {
    Array<Function*> functions;
    for (const auto& [typeName, typeFunctions] : GetTypeFunctionMap()) {
        functions += typeFunctions;
    }
    return functions;
}

Function* Function::FindFunction(const Class& InClass, const String& InName) {
    for (Function* function : GetAllTypeFunctions(InClass)) {
        if (function->Name == InName) {
            return function;
        }
    }
    return nullptr;
}

void* FunctionFrame::GetValuePtr(const String& InName) const {
    Property* property = m_Function.FindInput(InName);
    property = property ? property : m_Function.FindOutput(InName);
    AE_ASSERT(property, "'" + m_Function.Name + "' has no parameter named '" + InName + "'");
    return property->GetValuePtr(m_Params.get());
}
