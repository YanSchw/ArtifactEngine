#pragma once
#include "ScriptClass.h"
#include "GameFramework/Node.h"
#include <algorithm>
#include <new>

template<typename TProxy>
Object* AllocateScriptProxy(std::shared_ptr<const ScriptClassInfo> InInfo) {
    const uint32_t capacity = std::max(InInfo->StorageSize, ScriptStorageReserve);
    void* memory = ::operator new(InInfo->StorageOffset + capacity);
    return ::new (memory) TProxy(ScriptInstance{ std::move(InInfo), capacity });
}

class ScriptObjectProxy final : public Object {
public:
    explicit ScriptObjectProxy(ScriptInstance InScript) : m_Script(std::move(InScript)) {
        ScriptBridge::Construct(*this, m_Script);
    }

    ~ScriptObjectProxy() override {
        ScriptBridge::Destruct(*this, m_Script);
    }

    static void operator delete(void* InMemory) { ::operator delete(InMemory); }

    virtual Class GetClass() const override { return ScriptBridge::ClassOf(m_Script); }
    virtual bool IsObjectOfClass(const Class& InType) const override { return GetClass().IsSubclassOf(InType); }

private:
    ScriptInstance m_Script;
};

template<typename TBase>
class ScriptNodeProxy final : public TBase {
public:
    explicit ScriptNodeProxy(ScriptInstance InScript) : m_Script(std::move(InScript)) {
        if (m_Script.Info->Ticks) {
            this->SetUpdateFlag(UpdateFlag::WorldUpdate);
        }
        ScriptBridge::Construct(*this, m_Script);
    }

    ~ScriptNodeProxy() override {
        ScriptBridge::Destruct(*this, m_Script);
    }

    static void operator delete(void* InMemory) { ::operator delete(InMemory); }

    virtual Class GetClass() const override { return ScriptBridge::ClassOf(m_Script); }
    virtual bool IsObjectOfClass(const Class& InType) const override { return GetClass().IsSubclassOf(InType); }

    virtual void BeginPlay() override {
        TBase::BeginPlay();
        ScriptBridge::Call(*this, ScriptCall::BeginPlay);
    }

    virtual void EndPlay() override {
        ScriptBridge::Call(*this, ScriptCall::EndPlay);
        TBase::EndPlay();
    }

    virtual void WorldUpdate(float InDeltaTime) override {
        TBase::WorldUpdate(InDeltaTime);
        ScriptBridge::Call(*this, ScriptCall::WorldUpdate, InDeltaTime);
    }

private:
    ScriptInstance m_Script;
};
