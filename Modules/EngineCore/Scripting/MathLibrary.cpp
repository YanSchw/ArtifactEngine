#include "MathLibrary.h"
#include "GameFramework/Node3D.h"
#include <algorithm>
#include <cmath>
#include <random>

static std::mt19937& Random() {
    static std::mt19937 s_Random(std::random_device{}());
    return s_Random;
}

float MathLibrary::AddFloat(float InA, float InB) { return InA + InB; }
float MathLibrary::SubtractFloat(float InA, float InB) { return InA - InB; }
float MathLibrary::MultiplyFloat(float InA, float InB) { return InA * InB; }
float MathLibrary::DivideFloat(float InA, float InB) { return InB != 0.0f ? InA / InB : 0.0f; }
float MathLibrary::MinFloat(float InA, float InB) { return std::min(InA, InB); }
float MathLibrary::MaxFloat(float InA, float InB) { return std::max(InA, InB); }
float MathLibrary::ClampFloat(float InValue, float InMin, float InMax) { return std::clamp(InValue, std::min(InMin, InMax), std::max(InMin, InMax)); }
float MathLibrary::AbsFloat(float InValue) { return std::abs(InValue); }
float MathLibrary::SignFloat(float InValue) { return (float)((InValue > 0.0f) - (InValue < 0.0f)); }
float MathLibrary::Lerp(float InA, float InB, float InAlpha) { return InA + (InB - InA) * InAlpha; }

float MathLibrary::MapRangeClamped(float InValue, float InFromMin, float InFromMax, float InToMin, float InToMax) {
    const float range = InFromMax - InFromMin;
    const float alpha = range != 0.0f ? std::clamp((InValue - InFromMin) / range, 0.0f, 1.0f) : 0.0f;
    return Lerp(InToMin, InToMax, alpha);
}

float MathLibrary::FInterpTo(float InCurrent, float InTarget, float InDeltaTime, float InSpeed) {
    if (InSpeed <= 0.0f) {
        return InTarget;
    }
    return InCurrent + (InTarget - InCurrent) * std::clamp(InDeltaTime * InSpeed, 0.0f, 1.0f);
}

float MathLibrary::Sqrt(float InValue) { return InValue > 0.0f ? std::sqrt(InValue) : 0.0f; }
float MathLibrary::Power(float InBase, float InExponent) { return std::pow(InBase, InExponent); }
int32_t MathLibrary::FloorToInt(float InValue) { return (int32_t)std::floor(InValue); }
int32_t MathLibrary::RoundToInt(float InValue) { return (int32_t)std::round(InValue); }
int32_t MathLibrary::CeilToInt(float InValue) { return (int32_t)std::ceil(InValue); }
bool MathLibrary::GreaterFloat(float InA, float InB) { return InA > InB; }
bool MathLibrary::LessFloat(float InA, float InB) { return InA < InB; }
bool MathLibrary::NearlyEqualFloat(float InA, float InB, float InTolerance) { return std::abs(InA - InB) <= InTolerance; }

float MathLibrary::Sin(float InRadians) { return std::sin(InRadians); }
float MathLibrary::Cos(float InRadians) { return std::cos(InRadians); }
float MathLibrary::Tan(float InRadians) { return std::tan(InRadians); }
float MathLibrary::Atan2(float InY, float InX) { return std::atan2(InY, InX); }
float MathLibrary::DegreesToRadians(float InDegrees) { return glm::radians(InDegrees); }
float MathLibrary::RadiansToDegrees(float InRadians) { return glm::degrees(InRadians); }

int32_t MathLibrary::AddInt(int32_t InA, int32_t InB) { return InA + InB; }
int32_t MathLibrary::SubtractInt(int32_t InA, int32_t InB) { return InA - InB; }
int32_t MathLibrary::MultiplyInt(int32_t InA, int32_t InB) { return InA * InB; }
int32_t MathLibrary::DivideInt(int32_t InA, int32_t InB) { return InB != 0 ? InA / InB : 0; }
int32_t MathLibrary::Modulo(int32_t InA, int32_t InB) { return InB != 0 ? InA % InB : 0; }
int32_t MathLibrary::MinInt(int32_t InA, int32_t InB) { return std::min(InA, InB); }
int32_t MathLibrary::MaxInt(int32_t InA, int32_t InB) { return std::max(InA, InB); }
int32_t MathLibrary::ClampInt(int32_t InValue, int32_t InMin, int32_t InMax) { return std::clamp(InValue, std::min(InMin, InMax), std::max(InMin, InMax)); }
int32_t MathLibrary::AbsInt(int32_t InValue) { return std::abs(InValue); }
float MathLibrary::IntToFloat(int32_t InValue) { return (float)InValue; }
bool MathLibrary::GreaterInt(int32_t InA, int32_t InB) { return InA > InB; }
bool MathLibrary::LessInt(int32_t InA, int32_t InB) { return InA < InB; }
bool MathLibrary::EqualInt(int32_t InA, int32_t InB) { return InA == InB; }

bool MathLibrary::And(bool InA, bool InB) { return InA && InB; }
bool MathLibrary::Or(bool InA, bool InB) { return InA || InB; }
bool MathLibrary::Not(bool InValue) { return !InValue; }
bool MathLibrary::Xor(bool InA, bool InB) { return InA != InB; }

void MathLibrary::BreakVec3(const Vec3& InVector, float& OutX, float& OutY, float& OutZ) {
    OutX = InVector.x;
    OutY = InVector.y;
    OutZ = InVector.z;
}

Vec2 MathLibrary::MakeVec2(float InX, float InY) { return Vec2(InX, InY); }

void MathLibrary::BreakVec2(const Vec2& InVector, float& OutX, float& OutY) {
    OutX = InVector.x;
    OutY = InVector.y;
}

Vec3 MathLibrary::AddVec3(const Vec3& InA, const Vec3& InB) { return InA + InB; }
Vec3 MathLibrary::SubtractVec3(const Vec3& InA, const Vec3& InB) { return InA - InB; }
Vec3 MathLibrary::ScaleVec3(const Vec3& InVector, float InScale) { return InVector * InScale; }
Vec3 MathLibrary::NegateVec3(const Vec3& InVector) { return -InVector; }
float MathLibrary::Dot(const Vec3& InA, const Vec3& InB) { return glm::dot(InA, InB); }
Vec3 MathLibrary::Cross(const Vec3& InA, const Vec3& InB) { return glm::cross(InA, InB); }
float MathLibrary::VectorLength(const Vec3& InVector) { return glm::length(InVector); }
float MathLibrary::Distance(const Vec3& InA, const Vec3& InB) { return glm::distance(InA, InB); }

Vec3 MathLibrary::Normalize(const Vec3& InVector) {
    const float length = glm::length(InVector);
    return length > 0.0f ? InVector / length : Vec3(0.0f);
}

Vec3 MathLibrary::LerpVec3(const Vec3& InA, const Vec3& InB, float InAlpha) { return glm::mix(InA, InB, InAlpha); }

Vec3 MathLibrary::VInterpTo(const Vec3& InCurrent, const Vec3& InTarget, float InDeltaTime, float InSpeed) {
    if (InSpeed <= 0.0f) {
        return InTarget;
    }
    return glm::mix(InCurrent, InTarget, std::clamp(InDeltaTime * InSpeed, 0.0f, 1.0f));
}

Vec3 MathLibrary::DirectionBetween(const Vec3& InFrom, const Vec3& InTo) { return Normalize(InTo - InFrom); }

float MathLibrary::DistanceBetweenNodes(Node3D* InA, Node3D* InB) {
    return InA && InB ? glm::distance(InA->GetPosition(), InB->GetPosition()) : 0.0f;
}

Color MathLibrary::MakeColor(float InR, float InG, float InB, float InA) { return Color(InR, InG, InB, InA); }
Color MathLibrary::LerpColor(const Color& InA, const Color& InB, float InAlpha) { return glm::mix(InA, InB, InAlpha); }

float MathLibrary::RandomFloat() { return std::uniform_real_distribution<float>(0.0f, 1.0f)(Random()); }

float MathLibrary::RandomFloatInRange(float InMin, float InMax) {
    return std::uniform_real_distribution<float>(std::min(InMin, InMax), std::max(InMin, InMax))(Random());
}

int32_t MathLibrary::RandomIntInRange(int32_t InMin, int32_t InMax) {
    return std::uniform_int_distribution<int32_t>(std::min(InMin, InMax), std::max(InMin, InMax))(Random());
}

bool MathLibrary::RandomBool() { return std::bernoulli_distribution(0.5)(Random()); }

Vec3 MathLibrary::RandomUnitVector() {
    std::normal_distribution<float> normal(0.0f, 1.0f);
    return Normalize(Vec3(normal(Random()), normal(Random()), normal(Random())));
}
