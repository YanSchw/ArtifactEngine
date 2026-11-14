#include "NodeReference.h"
#include "Node.h"
#include <algorithm>

Node* NodeReference::Resolve(Node& InFrom) const {
    if (Path.empty()) {
        return nullptr;
    }
    Node* node = &InFrom;
    size_t start = 0;
    while (node && start <= Path.size()) {
        const size_t end = std::min(Path.find('/', start), Path.size());
        const String segment = Path.substr(start, end - start);
        if (segment == Parent) {
            node = node->GetParent();
        } else if (!segment.empty() && segment != Self) {
            node = node->GetDirectChildByName(segment);
        }
        start = end + 1;
    }
    return node;
}

NodeReference NodeReference::Between(Node& InFrom, Node& InTarget) {
    if (&InFrom == &InTarget) {
        return NodeReference(Self);
    }

    Array<Node*> ancestors;
    for (Node* node = &InFrom; node; node = node->GetParent()) {
        ancestors.Add(node);
    }

    Array<String> down;
    Node* common = &InTarget;
    while (common && !ancestors.Contains(common)) {
        down.Insert(0, common->GetName());
        common = common->GetParent();
    }
    if (!common) {
        return NodeReference();
    }

    String path;
    for (int32_t i = 0; i < ancestors.IndexOf(common); i++) {
        path += (path.empty() ? "" : "/") + Parent;
    }
    for (const String& name : down) {
        path += (path.empty() ? "" : "/") + name;
    }
    return NodeReference(path);
}
