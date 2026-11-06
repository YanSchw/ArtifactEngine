#include "AudioListenerNode.h"
#include "AudioWorld.h"
#include "World.h"

static AudioWorld* GetAudioWorld(const Node& InNode) {
    World* world = InNode.GetWorld();
    return world ? world->GetSubsystem<AudioWorld>() : nullptr;
}

void AudioListenerNode::BeginPlay() {
    Super::BeginPlay();
    Activate();
}

void AudioListenerNode::EndPlay() {
    AudioWorld* audio = GetAudioWorld(*this);
    if (audio && audio->GetListener() == this) {
        audio->SetListener(nullptr);
    }
    Super::EndPlay();
}

void AudioListenerNode::Activate() {
    if (AudioWorld* audio = GetAudioWorld(*this)) {
        audio->SetListener(this);
    }
}

bool AudioListenerNode::IsActive() const {
    AudioWorld* audio = GetAudioWorld(*this);
    return audio && audio->GetListener() == this;
}
