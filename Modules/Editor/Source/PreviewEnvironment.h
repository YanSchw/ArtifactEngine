#pragma once
#include "GameFramework/Node.h"
#include "PreviewEnvironment.gen.h"

enum class AuthoredNodes : uint8_t { Enabled, All };

class PreviewEnvironment : public Node {
public:
    ARTIFACT_CLASS();

    PreviewEnvironment();

    static bool Contains(Node& InNode);

    void YieldTo(AuthoredNodes InAuthored);
};
