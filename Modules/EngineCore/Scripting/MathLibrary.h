#pragma once
#include "Common/Types.h"
#include "Object/Object.h"
#include "MathLibrary.gen.h"

class Node3D;

class MathLibrary : public Object {
public:
    ARTIFACT_CLASS();

    FUNCTION(Pure, Category = "Math|Float")
    static float AddFloat(float InA, float InB);
    FUNCTION(Pure, Category = "Math|Float")
    static float SubtractFloat(float InA, float InB);
    FUNCTION(Pure, Category = "Math|Float")
    static float MultiplyFloat(float InA, float InB);
    FUNCTION(Pure, Category = "Math|Float")
    static float DivideFloat(float InA, float InB = 1.0f);
    FUNCTION(Pure, Category = "Math|Float")
    static float MinFloat(float InA, float InB);
    FUNCTION(Pure, Category = "Math|Float")
    static float MaxFloat(float InA, float InB);
    FUNCTION(Pure, Category = "Math|Float")
    static float ClampFloat(float InValue, float InMin = 0.0f, float InMax = 1.0f);
    FUNCTION(Pure, Category = "Math|Float")
    static float AbsFloat(float InValue);
    FUNCTION(Pure, Category = "Math|Float")
    static float SignFloat(float InValue);
    FUNCTION(Pure, Category = "Math|Float")
    static float Lerp(float InA, float InB, float InAlpha);
    FUNCTION(Pure, Category = "Math|Float")
    static float MapRangeClamped(float InValue, float InFromMin, float InFromMax, float InToMin, float InToMax);
    FUNCTION(Pure, Category = "Math|Float")
    static float FInterpTo(float InCurrent, float InTarget, float InDeltaTime, float InSpeed);
    FUNCTION(Pure, Category = "Math|Float")
    static float Sqrt(float InValue);
    FUNCTION(Pure, Category = "Math|Float")
    static float Power(float InBase, float InExponent);
    FUNCTION(Pure, Category = "Math|Float")
    static int32_t FloorToInt(float InValue);
    FUNCTION(Pure, Category = "Math|Float")
    static int32_t RoundToInt(float InValue);
    FUNCTION(Pure, Category = "Math|Float")
    static int32_t CeilToInt(float InValue);
    FUNCTION(Pure, Category = "Math|Float")
    static bool GreaterFloat(float InA, float InB);
    FUNCTION(Pure, Category = "Math|Float")
    static bool LessFloat(float InA, float InB);
    FUNCTION(Pure, Category = "Math|Float")
    static bool NearlyEqualFloat(float InA, float InB, float InTolerance = 0.0001f);

    FUNCTION(Pure, Category = "Math|Trigonometry")
    static float Sin(float InRadians);
    FUNCTION(Pure, Category = "Math|Trigonometry")
    static float Cos(float InRadians);
    FUNCTION(Pure, Category = "Math|Trigonometry")
    static float Tan(float InRadians);
    FUNCTION(Pure, Category = "Math|Trigonometry")
    static float Atan2(float InY, float InX);
    FUNCTION(Pure, Category = "Math|Trigonometry")
    static float DegreesToRadians(float InDegrees);
    FUNCTION(Pure, Category = "Math|Trigonometry")
    static float RadiansToDegrees(float InRadians);

    FUNCTION(Pure, Category = "Math|Int")
    static int32_t AddInt(int32_t InA, int32_t InB);
    FUNCTION(Pure, Category = "Math|Int")
    static int32_t SubtractInt(int32_t InA, int32_t InB);
    FUNCTION(Pure, Category = "Math|Int")
    static int32_t MultiplyInt(int32_t InA, int32_t InB);
    FUNCTION(Pure, Category = "Math|Int")
    static int32_t DivideInt(int32_t InA, int32_t InB = 1);
    FUNCTION(Pure, Category = "Math|Int")
    static int32_t Modulo(int32_t InA, int32_t InB = 1);
    FUNCTION(Pure, Category = "Math|Int")
    static int32_t MinInt(int32_t InA, int32_t InB);
    FUNCTION(Pure, Category = "Math|Int")
    static int32_t MaxInt(int32_t InA, int32_t InB);
    FUNCTION(Pure, Category = "Math|Int")
    static int32_t ClampInt(int32_t InValue, int32_t InMin, int32_t InMax);
    FUNCTION(Pure, Category = "Math|Int")
    static int32_t AbsInt(int32_t InValue);
    FUNCTION(Pure, Category = "Math|Int")
    static float IntToFloat(int32_t InValue);
    FUNCTION(Pure, Category = "Math|Int")
    static bool GreaterInt(int32_t InA, int32_t InB);
    FUNCTION(Pure, Category = "Math|Int")
    static bool LessInt(int32_t InA, int32_t InB);
    FUNCTION(Pure, Category = "Math|Int")
    static bool EqualInt(int32_t InA, int32_t InB);

    FUNCTION(Pure, Category = "Math|Boolean")
    static bool And(bool InA, bool InB);
    FUNCTION(Pure, Category = "Math|Boolean")
    static bool Or(bool InA, bool InB);
    FUNCTION(Pure, Category = "Math|Boolean")
    static bool Not(bool InValue);
    FUNCTION(Pure, Category = "Math|Boolean")
    static bool Xor(bool InA, bool InB);

    FUNCTION(Pure, Category = "Math|Vector")
    static void BreakVec3(const Vec3& InVector, float& OutX, float& OutY, float& OutZ);
    FUNCTION(Pure, Category = "Math|Vector")
    static Vec2 MakeVec2(float InX, float InY);
    FUNCTION(Pure, Category = "Math|Vector")
    static void BreakVec2(const Vec2& InVector, float& OutX, float& OutY);
    FUNCTION(Pure, Category = "Math|Vector")
    static Vec3 AddVec3(const Vec3& InA, const Vec3& InB);
    FUNCTION(Pure, Category = "Math|Vector")
    static Vec3 SubtractVec3(const Vec3& InA, const Vec3& InB);
    FUNCTION(Pure, Category = "Math|Vector")
    static Vec3 ScaleVec3(const Vec3& InVector, float InScale);
    FUNCTION(Pure, Category = "Math|Vector")
    static Vec3 NegateVec3(const Vec3& InVector);
    FUNCTION(Pure, Category = "Math|Vector")
    static float Dot(const Vec3& InA, const Vec3& InB);
    FUNCTION(Pure, Category = "Math|Vector")
    static Vec3 Cross(const Vec3& InA, const Vec3& InB);
    FUNCTION(Pure, Category = "Math|Vector")
    static float VectorLength(const Vec3& InVector);
    FUNCTION(Pure, Category = "Math|Vector")
    static float Distance(const Vec3& InA, const Vec3& InB);
    FUNCTION(Pure, Category = "Math|Vector")
    static Vec3 Normalize(const Vec3& InVector);
    FUNCTION(Pure, Category = "Math|Vector")
    static Vec3 LerpVec3(const Vec3& InA, const Vec3& InB, float InAlpha);
    FUNCTION(Pure, Category = "Math|Vector")
    static Vec3 VInterpTo(const Vec3& InCurrent, const Vec3& InTarget, float InDeltaTime, float InSpeed);
    FUNCTION(Pure, Category = "Math|Vector")
    static Vec3 DirectionBetween(const Vec3& InFrom, const Vec3& InTo);
    FUNCTION(Pure, Category = "Math|Vector")
    static float DistanceBetweenNodes(Node3D* InA, Node3D* InB);

    FUNCTION(Pure, Category = "Math|Color")
    static Color MakeColor(float InR = 1.0f, float InG = 1.0f, float InB = 1.0f, float InA = 1.0f);
    FUNCTION(Pure, Category = "Math|Color")
    static Color LerpColor(const Color& InA, const Color& InB, float InAlpha);

    FUNCTION(Category = "Math|Random")
    static float RandomFloat();
    FUNCTION(Category = "Math|Random")
    static float RandomFloatInRange(float InMin = 0.0f, float InMax = 1.0f);
    FUNCTION(Category = "Math|Random")
    static int32_t RandomIntInRange(int32_t InMin = 0, int32_t InMax = 10);
    FUNCTION(Category = "Math|Random")
    static bool RandomBool();
    FUNCTION(Category = "Math|Random")
    static Vec3 RandomUnitVector();
};
