#pragma once
#include "Common/Types.h"
#include "Object/Object.h"
#include "Object/SubclassOf.h"
#include "ScriptLibrary.gen.h"

class Node;

class ScriptLibrary : public Object {
public:
    ARTIFACT_CLASS();

    FUNCTION(Category = "Utilities")
    static void PrintString(const String& InText);

    FUNCTION(Category = "Utilities")
    static void PrintWarning(const String& InText);

    FUNCTION(Pure, Category = "Utilities")
    static Vec3 MakeVec3(float InX, float InY, float InZ);

    FUNCTION(Pure, Category = "Utilities")
    static bool IsValid(Object* InObject);

    FUNCTION(Pure, Category = "Utilities")
    static String GetClassName(Object* InObject);

    FUNCTION(Pure, Category = "Utilities|String")
    static String Append(const String& InA, const String& InB);
    FUNCTION(Pure, Category = "Utilities|String")
    static bool Contains(const String& InText, const String& InSubstring);
    FUNCTION(Pure, Category = "Utilities|String")
    static bool EqualString(const String& InA, const String& InB);
    FUNCTION(Pure, Category = "Utilities|String")
    static int32_t StringLength(const String& InText);
    FUNCTION(Pure, Category = "Utilities|String")
    static String FloatToString(float InValue);
    FUNCTION(Pure, Category = "Utilities|String")
    static String IntToString(int32_t InValue);
    FUNCTION(Pure, Category = "Utilities|String")
    static String BoolToString(bool InValue);
    FUNCTION(Pure, Category = "Utilities|String")
    static String Vec3ToString(const Vec3& InValue);

    FUNCTION(Pure, Category = "Scene")
    static Node* FindNodeByName(Node* InContext, const String& InName);
    FUNCTION(Category = "Scene")
    static Node* SpawnNode(Node* InContext, SubclassOf<Node> InClass);
    FUNCTION(Category = "Scene")
    static Node* SpawnChild(Node* InParent, SubclassOf<Node> InClass);
};
