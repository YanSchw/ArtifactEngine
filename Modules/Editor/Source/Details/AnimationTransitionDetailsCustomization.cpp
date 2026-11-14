#include "AnimationTransitionDetailsCustomization.h"
#include "AnimationTransitionPreview.h"
#include "DetailsCategory.h"
#include "DetailsRow.h"
#include "Tabs/DetailsTab.h"
#include "Tabs/MajorTab.h"
#include "UI/EditorIcons.h"
#include "UI/EditorStyle.h"
#include "UI/UIDragNumber.h"
#include "UI/UIDropdown.h"
#include "GameFramework/UIButton.h"
#include "GameFramework/UIHStack.h"
#include "GameFramework/UILabel.h"
#include "GameFramework/UISvg.h"
#include "Assets/AnimationGraph.h"

using ConditionEdit = std::function<void(AnimationTransition&)>;

static AnimationGraph* GetEditedGraph(DetailsTab& InTab) {
    MajorTab* major = InTab.GetMajorTab();
    return major ? Cast<AnimationGraph>(major->GetEditedAsset()) : nullptr;
}

static Array<AnimationConditionMode> ModesFor(AnimationParameterType InType) {
    switch (InType) {
        case AnimationParameterType::Float:   return { AnimationConditionMode::Greater, AnimationConditionMode::Less };
        case AnimationParameterType::Int:     return { AnimationConditionMode::Greater, AnimationConditionMode::Less,
                                                       AnimationConditionMode::Equals, AnimationConditionMode::NotEqual };
        case AnimationParameterType::Bool:    return { AnimationConditionMode::If, AnimationConditionMode::IfNot };
        case AnimationParameterType::Trigger: return { AnimationConditionMode::If };
    }
    return {};
}

static String ModeLabel(AnimationConditionMode InMode, AnimationParameterType InType) {
    if (InType == AnimationParameterType::Bool) {
        return InMode == AnimationConditionMode::If ? "true" : "false";
    }
    return Enum("AnimationConditionMode").ConvertValueToString((int64_t)InMode);
}

float AnimationTransitionDetailsCustomization::BuildHeader(UINode& InHeader, Object* InObject, DetailsTab& InTab) {
    AnimationTransition* transition = Cast<AnimationTransition>(InObject);
    AnimationGraph* graph = GetEditedGraph(InTab);
    AnimationStateMachine* machine = graph ? graph->GetStateMachine() : nullptr;
    GraphNode* from = machine ? machine->FindNode(transition->FromNodeId) : nullptr;
    GraphNode* to = machine ? machine->FindNode(transition->ToNodeId) : nullptr;

    UILabel* label = InHeader.Add<UILabel>();
    label->Fill();
    label->Position = Vec2(10.0f, 0.0f);
    label->FontSize = EditorStyle::FontSize + 1.0f;
    label->Color = EditorStyle::TextBright;
    label->VAlign = UIVAlign::Middle;
    label->Text = (from ? from->GetTitle() : String("?")) + "  ->  " + (to ? to->GetTitle() : String("?"));
    return 32.0f;
}

void AnimationTransitionDetailsCustomization::BuildContent(UINode& InList, Object* InObject, DetailsTab& InTab) {
    AnimationTransition* transition = Cast<AnimationTransition>(InObject);
    AnimationGraph* graph = GetEditedGraph(InTab);
    AnimationStateMachine* machine = graph ? graph->GetStateMachine() : nullptr;
    if (!transition || !machine || Cast<AnimationEntryNode>(machine->FindNode(transition->FromNodeId))) {
        return;
    }

    const WeakObjectPtr<Object> weak(InObject);
    DetailsCategory& settings = AddCategory(InList, InTab, "Settings", 0);
    const auto addRow = [&](const char* InName, const String& InLabel = String()) {
        AddPropertyRow(*settings.GetBody(), InTab, weak, 0, Property::FindTypeProperty(AnimationTransition::StaticClass(), InName), 0,
                       nullptr, InLabel);
    };
    addRow("HasExitTime");
    if (transition->HasExitTime) {
        addRow("ExitTime");
    }
    addRow("FixedDuration");
    addRow("Duration", transition->FixedDuration ? "Transition Duration (s)" : "Transition Duration (%)");
    addRow("Offset", "Transition Offset");
    addRow("InterruptionSource");
    const AnimationInterruptionSource interruption = transition->InterruptionSource;
    if (interruption != AnimationInterruptionSource::None && interruption != AnimationInterruptionSource::NextState) {
        addRow("OrderedInterruption");
    }

    AnimationTransitionPreview* preview = settings.GetBody()->Add<AnimationTransitionPreview>();
    preview->OwnerTab = &InTab;
    preview->Transition = transition;
    preview->Graph = graph;

    const WeakObjectPtr<DetailsTab> tab = &InTab;
    const WeakObjectPtr<AnimationGraph> weakGraph = graph;
    const auto edit = [tab, weak](const String& InTitle, bool InRebuild, ConditionEdit InEdit) {
        DetailsTab* details = tab.Get();
        AnimationTransition* target = Cast<AnimationTransition>(weak.Get());
        if (!details || !target) {
            return;
        }
        details->RecordEdit(InTitle, target);
        InEdit(*target);
        NotifyPropertyEdited(details, target, "Conditions");
        if (InRebuild) {
            details->MarkDirty();
        }
    };
    const auto parameterOf = [weakGraph, weak](int32_t InIndex) -> const AnimationParameter* {
        AnimationTransition* target = Cast<AnimationTransition>(weak.Get());
        AnimationGraph* owner = weakGraph.Get();
        return target && owner && InIndex < target->Conditions.Size() ? owner->FindParameter(target->Conditions[InIndex].Parameter) : nullptr;
    };

    DetailsCategory& conditions = AddCategory(InList, InTab, "Conditions", 0);
    UIButton* add = conditions.AddHeaderValue()->Add<UIButton>();
    add->Anchor = add->Pivot = Vec2(1.0f, 0.5f);
    add->Size = Vec2(20.0f, 20.0f);
    add->NormalColor = Vec4(0.0f);
    add->HoverColor = EditorStyle::TabHover;
    UISvg* addIcon = add->Add<UISvg>();
    addIcon->Center(Vec2(12.0f, 12.0f));
    addIcon->Image = EditorIcons::Add();
    addIcon->Tint = EditorStyle::TextDim;
    add->Clicked = [edit, weakGraph] {
        edit("Add Condition", true, [weakGraph](AnimationTransition& OutTransition) {
            AnimationCondition condition;
            AnimationGraph* owner = weakGraph.Get();
            if (owner && !owner->GetParameters().IsEmpty()) {
                const AnimationParameter& first = owner->GetParameters()[0];
                condition.Parameter = first.Name;
                condition.Mode = ModesFor(first.Type)[0];
            }
            OutTransition.Conditions.Add(condition);
        });
    };

    if (transition->Conditions.IsEmpty()) {
        AddRow(*conditions.GetBody(), InTab, transition->HasExitTime ? "Leaves at its exit time" : "Leaves right away", 0);
    }

    for (int32_t index = 0; index < transition->Conditions.Size(); index++) {
        DetailsRow& row = AddRow(*conditions.GetBody(), InTab, transition->Conditions[index].Parameter, 0);

        UIDropdown* parameter = row.AddLabelHost()->Add<UIDropdown>();
        parameter->Fill();
        parameter->GetSelectedLabel = [weak, parameterOf, index]() -> String {
            AnimationTransition* target = Cast<AnimationTransition>(weak.Get());
            if (!target || index >= target->Conditions.Size()) {
                return String();
            }
            const String& name = target->Conditions[index].Parameter;
            return parameterOf(index) ? name : name.empty() ? String("None") : "Missing: " + name;
        };
        parameter->GetOptions = [weakGraph] {
            Array<UIDropdownOption> options;
            if (AnimationGraph* owner = weakGraph.Get()) {
                for (const AnimationParameter& candidate : owner->GetParameters()) {
                    options.Add(candidate.Name);
                }
            }
            return options;
        };
        parameter->GetSelectedIndex = [weakGraph, parameterOf, index]() -> int32_t {
            const AnimationParameter* current = parameterOf(index);
            AnimationGraph* owner = weakGraph.Get();
            for (int32_t option = 0; current && owner && option < owner->GetParameters().Size(); option++) {
                if (&owner->GetParameters()[option] == current) {
                    return option;
                }
            }
            return -1;
        };
        parameter->SelectionChanged = [edit, weakGraph, index](int32_t InOption) {
            AnimationGraph* owner = weakGraph.Get();
            if (!owner || InOption < 0 || InOption >= owner->GetParameters().Size()) {
                return;
            }
            const AnimationParameter chosen = owner->GetParameters()[InOption];
            edit("Edit Condition", true, [chosen, index](AnimationTransition& OutTransition) {
                if (index < OutTransition.Conditions.Size()) {
                    OutTransition.Conditions[index].Parameter = chosen.Name;
                    OutTransition.Conditions[index].Mode = ModesFor(chosen.Type)[0];
                }
            });
        };

        const AnimationParameter* current = parameterOf(index);
        const AnimationParameterType type = current ? current->Type : AnimationParameterType::Trigger;

        UIHStack* value = row.GetValueHost()->Add<UIHStack>();
        value->Fill();
        value->Gap = 4.0f;

        if (ModesFor(type).Size() > 1) {
            UIDropdown* mode = value->Add<UIDropdown>();
            mode->Size = { 1.0_rel, 1.0_rel };
            mode->GetSelectedLabel = [weak, index, type]() -> String {
                AnimationTransition* target = Cast<AnimationTransition>(weak.Get());
                return target && index < target->Conditions.Size() ? ModeLabel(target->Conditions[index].Mode, type) : String();
            };
            mode->GetOptions = [type] {
                Array<UIDropdownOption> options;
                for (AnimationConditionMode candidate : ModesFor(type)) {
                    options.Add(ModeLabel(candidate, type));
                }
                return options;
            };
            mode->GetSelectedIndex = [weak, index, type]() -> int32_t {
                AnimationTransition* target = Cast<AnimationTransition>(weak.Get());
                return target && index < target->Conditions.Size() ? ModesFor(type).IndexOf(target->Conditions[index].Mode) : -1;
            };
            mode->SelectionChanged = [edit, index, type](int32_t InOption) {
                const Array<AnimationConditionMode> modes = ModesFor(type);
                if (InOption < 0 || InOption >= modes.Size()) {
                    return;
                }
                edit("Edit Condition", false, [mode = modes[InOption], index](AnimationTransition& OutTransition) {
                    if (index < OutTransition.Conditions.Size()) {
                        OutTransition.Conditions[index].Mode = mode;
                    }
                });
            };
        } else {
            value->Add<UINode>()->Size = { 1.0_rel, 1.0_rel };
        }

        if (type == AnimationParameterType::Float || type == AnimationParameterType::Int) {
            UIDragNumber* threshold = value->Add<UIDragNumber>();
            threshold->Size = { 64.0_px, 1.0_rel };
            threshold->Integer = type == AnimationParameterType::Int;
            threshold->Get = [weak, index]() -> double {
                AnimationTransition* target = Cast<AnimationTransition>(weak.Get());
                return target && index < target->Conditions.Size() ? (double)target->Conditions[index].Threshold : 0.0;
            };
            threshold->Set = [edit, index](double InValue) {
                edit("Edit Condition", false, [index, InValue](AnimationTransition& OutTransition) {
                    if (index < OutTransition.Conditions.Size()) {
                        OutTransition.Conditions[index].Threshold = (float)InValue;
                    }
                });
            };
        }

        UIButton* remove = value->Add<UIButton>();
        remove->Size = { 20.0_px, 1.0_rel };
        remove->NormalColor = Vec4(0.0f);
        remove->HoverColor = EditorStyle::TabHover;
        UISvg* removeIcon = remove->Add<UISvg>();
        removeIcon->Center(Vec2(12.0f, 12.0f));
        removeIcon->Image = EditorIcons::Delete();
        removeIcon->Tint = EditorStyle::TextDim;
        remove->Clicked = [edit, index] {
            edit("Remove Condition", true, [index](AnimationTransition& OutTransition) {
                if (index < OutTransition.Conditions.Size()) {
                    OutTransition.Conditions.RemoveAt(index);
                }
            });
        };
    }
}
