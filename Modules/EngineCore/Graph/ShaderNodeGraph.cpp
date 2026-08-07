#include "ShaderNodeGraph.h"

#include "Graph/ShaderGraphNode.h"

bool ShaderNodeGraph::ArePinTypesCompatible(const String& InOutputType, const String& InInputType) const {
    ShaderValueType output = ShaderValueType::Float;
    ShaderValueType input = ShaderValueType::Float;
    return ShaderValue::ParseGlslType(InOutputType, output)
        && ShaderValue::ParseGlslType(InInputType, input);
}

bool ShaderNodeGraph::AllowsNodeClass(const Class& InNodeClass) const {
    return InNodeClass != ShaderGraphNode::StaticClass()
        && InNodeClass.IsSubclassOf(ShaderGraphNode::StaticClass());
}
