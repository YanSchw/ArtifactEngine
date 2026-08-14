#pragma once
#include "MajorTab.h"
#include "Object/Pointer.h"
#include "AnimationEditorTab.gen.h"

class Animation;
class AnimationTimelineTab;
class Node;
class NodeRecord;
class ViewportTab;
class VectorImage;

/** A node's property values as they were before previewing, so leaving preview puts the
 *  placeholder back the way the user built it. */
struct AnimationPose {
    WeakObjectPtr<Node> Target;
    String Values;
};

/** Edits one Animation. */
class AnimationEditorTab : public MajorTab {
public:
    ARTIFACT_CLASS();

    AnimationEditorTab();

    void OpenAnimation(Animation* InAnimation);
    Animation* GetAnimation() const { return m_Animation.Get(); }
    void Save();

    Class GetPlaceholderClass() const;
    void SetPlaceholderClass(const Class& InClass);
    Array<Class> GetSelectablePlaceholderClasses() const;

    Node* GetAnimationRoot() const;
    void SetAnimationRoot(Node* InNode);
    bool IsAnimationRoot(Node* InNode) const;

    /** While previewing, the placeholder is posed by the Animation at the playhead. */
    bool IsPreviewing() const { return m_Previewing; }
    void SetPreviewing(bool InPreviewing);

    bool IsRecording() const { return m_Recording; }
    void SetRecording(bool InRecording);

    bool IsPlaying() const { return m_Playing; }
    void SetPlaying(bool InPlaying);

    /** Records InPropertyName of InNode at the playhead. A property that is being animated for the
     *  first time also gets a key at frame 0 holding what it had before previewing, so everything
     *  ahead of the first key keeps the value the placeholder was authored with. */
    bool KeyProperty(Node& InNode, const String& InPropertyName);

    int32_t GetFrame() const;
    float GetFramePosition() const { return m_FramePosition; }
    void SetFrame(int32_t InFrame);

    virtual String GetTabTitle() const override;
    virtual VectorImage* GetTabIcon() const override;
    virtual void BuildToolBar(UINode& InToolBar) override;
    virtual Asset* GetEditedAsset() const override;
    virtual Node* GetAssetRootNode() const override;
    virtual void OnAssetSaved(Asset* InAsset) override;
    virtual void OnPropertyEdited(Object* InObject, const String& InPropertyName) override;
    virtual void OnUIUpdate(const UIFrameContext& InContext) override;

private:
    void DestroyInstance();
    void BuildInstanceFrom(NodeRecord& InRecord);
    void ApplyCurrentFrame();
    void CapturePose();
    void RestorePose();
    void SeedTrackStart(Node& InNode, const String& InPropertyName, const Array<int32_t>& InPath);

    WeakObjectPtr<Animation> m_Animation;
    WeakObjectPtr<Node> m_Instance;
    WeakObjectPtr<Node> m_AnimationRoot;
    ViewportTab* m_Viewport = nullptr;
    AnimationTimelineTab* m_Timeline = nullptr;

    Array<AnimationPose> m_PreviewPose;
    float m_FramePosition = 0.0f;
    bool m_Previewing = false;
    bool m_Recording = false;
    bool m_Playing = false;
};
