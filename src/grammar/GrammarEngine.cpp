#include "GrammarEngine.h"

#include <iostream>

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
    this->root = ShapeNode{};
    this->root.symbol = Symbol::Facade;
    this->root.position = V3(0, 0, -3.0f);
    this->root.size = V3(4.0f, 4.0f, 0.2f);
    this->root.isTerminal = false;
    this->root.id = 0;

    // Rules determine all bays, windows, and terminal geometry.
    GenerateTree(&this->root);
    PrintTree(&this->root);
    terminals.clear();
    CollectTerminals(&this->root, V3(0, 0, 0));
    PrintTerminals();
}

// generate grammar tree for a given preset
void GrammarEngine::GenerateTree(ShapeNode* node) {
    if (node == nullptr) {
        std::cerr << "Error: GenerateTree called with null node." << std::endl;
        return;
    }
    if (node->isTerminal) {
        return;
    }

    // generate grammar subtree for this node
    rules.Expand(node);

    // recursively generate grammar tree for each child
    for (ShapeNode& child : node->children) {
        GenerateTree(&child);
    }
}

// collect the terminal nodes with their world pose
void GrammarEngine::CollectTerminals(ShapeNode* node, V3 parentPose) {
    if (node == nullptr) {
        std::cerr << "Error: CollectTerminals called with null node." << std::endl;
        return;
    }

    V3 newParentPose = parentPose + node->position;

    // If the node is terminal, add it to the terminals vector
    if (node->isTerminal) {
        // copy and transform to world pose first
        ShapeNode terminal = *node;
        terminal.position = newParentPose;
        terminals.push_back(terminal);
        return;
    }

    // If the node is not terminal, recursively collect terminals from its children
    for (ShapeNode& child : node->children) {
        CollectTerminals(&child, newParentPose);
    }
}

// print the grammar tree for debugging
void GrammarEngine::PrintTree(const ShapeNode* node, int depth) {
    if (node == nullptr) {
        std::cerr << "Error: PrintTree called with null node." << std::endl;
        return;
    }

    for (int i = 0; i < depth; i++) {
        std::cout << "  ";
    }
    std::cout << int(node->symbol) << " (ID: " << node->id << ")" << " (Position: " << node->position << ")" << " (Size: " << node->size << ")" << std::endl;

    for (const ShapeNode& child : node->children) {
        PrintTree(&child, depth + 1);
    }
}

// print the collected terminal nodes for debugging
void GrammarEngine::PrintTerminals() {
    std::cout << "Collected Terminal Nodes:" << std::endl;
    for (const ShapeNode& terminal : terminals) {
        std::cout << "ID: " << terminal.id
                  << ", Symbol: " << int(terminal.symbol)
                  << ", Material: " << int(terminal.material)
                  << ", Position: " << terminal.position
                  << ", Size: " << terminal.size
                  << std::endl;
    }
}
