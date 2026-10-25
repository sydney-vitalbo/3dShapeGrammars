#include "GrammarEngine.h"

#include <ostream>

GrammarEngine::GrammarEngine() {
    rules = Rules();
    terminals = std::vector<ShapeNode>();
}

GrammarEngine::~GrammarEngine() {
    terminals.clear();
}

// Expand the facade rule, build tree,
// append terminal shape nodes to terminals vector
void GrammarEngine::GenerateFacade() {
    ShapeNode root;
    root.symbol = Symbol::Facade;
    root.position = V3(0, 0, -3.0f);
    root.size = V3(0.5f, 0.5f, 0.2f);
    root.isTerminal = false;
    root.id = 0;

    // Expand the facade rule
    Expand(root);
}

// recursive expansion of any rule, used by presets above
void GrammarEngine::Expand(ShapeNode node) {
    if (node.isTerminal) {
        terminals.push_back(node);
        return;
    }

    std::vector<ShapeNode> children = rules.Expand(node);

    for (ShapeNode child : children) {
        // Position the child globally and continue expanding recursively
        child.position = V3(node.position[0] + child.position[0] * node.size[0],
                            node.position[1] + child.position[1] * node.size[1],
                            node.position[2] + child.position[2] * node.size[2]);
        child.size = V3(node.size[0] * child.size[0],
                        node.size[1] * child.size[1],
                        node.size[2] * child.size[2]);
        Expand(child);
    }
}
