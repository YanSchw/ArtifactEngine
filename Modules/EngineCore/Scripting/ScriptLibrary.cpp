#include "ScriptLibrary.h"
#include "Core/Log.h"
#include "GameFramework/Node.h"
#include "GameFramework/World.h"
#include <cstdio>

void ScriptLibrary::PrintString(const String& InText) {
    AE_INFO("{0}", InText);
}

void ScriptLibrary::PrintWarning(const String& InText) {
    AE_WARN("{0}", InText);
}

Vec3 ScriptLibrary::MakeVec3(float InX, float InY, float InZ) {
    return Vec3(InX, InY, InZ);
}

bool ScriptLibrary::IsValid(Object* InObject) {
    Node* node = Cast<Node>(InObject);
    return InObject && (!node || !node->IsPendingKill());
}

String ScriptLibrary::GetClassName(Object* InObject) {
    return InObject ? InObject->GetClass().GetDisplayName() : String();
}

String ScriptLibrary::Append(const String& InA, const String& InB) {
    return InA + InB;
}

bool ScriptLibrary::Contains(const String& InText, const String& InSubstring) {
    return InText.find(InSubstring) != String::npos;
}

bool ScriptLibrary::EqualString(const String& InA, const String& InB) {
    return InA == InB;
}

int32_t ScriptLibrary::StringLength(const String& InText) {
    return (int32_t)InText.size();
}

String ScriptLibrary::FloatToString(float InValue) {
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%g", InValue);
    return buffer;
}

String ScriptLibrary::IntToString(int32_t InValue) {
    return std::to_string(InValue);
}

String ScriptLibrary::BoolToString(bool InValue) {
    return InValue ? "true" : "false";
}

String ScriptLibrary::Vec3ToString(const Vec3& InValue) {
    return FloatToString(InValue.x) + ", " + FloatToString(InValue.y) + ", " + FloatToString(InValue.z);
}

Node* ScriptLibrary::FindNodeByName(Node* InContext, const String& InName) {
    Node* root = InContext ? InContext->GetRootNode() : nullptr;
    return root ? root->GetChildByName(InName, true) : nullptr;
}

Node* ScriptLibrary::SpawnNode(Node* InContext, SubclassOf<Node> InClass) {
    World* world = InContext ? InContext->GetWorld() : nullptr;
    return world && InClass.IsValid() ? world->Spawn(InClass) : nullptr;
}

Node* ScriptLibrary::SpawnChild(Node* InParent, SubclassOf<Node> InClass) {
    return InParent && InClass.IsValid() ? InParent->CreateChild(InClass) : nullptr;
}
