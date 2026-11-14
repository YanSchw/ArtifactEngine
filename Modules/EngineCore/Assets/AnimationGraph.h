#pragma once
#include "Asset.h"
#include "Animation/AnimationStateMachine.h"
#include "AnimationGraph.gen.h"

ARTIFACT_ENUM();
enum class AnimationParameterType : uint8_t {
    Float = 0,
    Int,
    Bool,
    Trigger
};

struct AnimationParameter {
    ARTIFACT_STRUCT();

    PROPERTY()
    String Name;

    PROPERTY()
    AnimationParameterType Type = AnimationParameterType::Float;

    PROPERTY()
    float DefaultValue = 0.0f;
};

class AnimationGraph : public Asset {
public:
    ARTIFACT_CLASS();

    static AnimationGraph* CreateEmpty(const String& InDirectory, const String& InName);

    AnimationStateMachine* GetStateMachine();

    const Array<AnimationParameter>& GetParameters() const { return m_Parameters; }
    const AnimationParameter* FindParameter(const String& InName) const;
    int32_t AddParameter(AnimationParameterType InType);
    void RenameParameter(int32_t InIndex, const String& InName);
    void SetParameterDefault(int32_t InIndex, float InValue);
    void RemoveParameter(int32_t InIndex);
    String MakeUniqueParameterName(const String& InBase) const;

    void CollectEventNames(Array<String>& OutNames) const;

    virtual bool IsLoaded() const override { return m_Loaded; }

protected:
    virtual void Load() override;
    virtual void Unload() override;

private:
    PROPERTY()
    Array<AnimationParameter> m_Parameters;

    PROPERTY()
    SharedObjectPtr<AnimationStateMachine> m_StateMachine;

    Array<AssetStreamHandle> m_Motions;
    bool m_Loaded = false;
};
