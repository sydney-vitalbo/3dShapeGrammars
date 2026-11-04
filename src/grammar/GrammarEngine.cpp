#include "GrammarEngine.h"

#include <iostream>
#include <stdexcept>
#include <utility>

GrammarEngine::GrammarEngine() {
    terminals = std::vector<ShapeNode>();
    roots = std::vector<ShapeNode>();
}

GrammarEngine::~GrammarEngine() {
    terminals.clear();
}

// Expand the facade rule, build tree,
// append terminal shape nodes to terminals vector
void GrammarEngine::GenerateTest() {
    ShapeNode root;
    root.symbol = Symbol::Facade;
    root.position = V3(0, 0, -3.0f);
    root.size = V3(4.0f, 4.0f, 0.2f);
    root.isTerminal = false;
    root.id = 0;

    // override with apartment node for testing
    //building_rules::Parameters params;
    // testing subdivison (splitting height into floors) and repetition (bays within floor)
    //params.floors = 3;
    //params.bays = 3;
    //root = building_rules::buildingRoot(params, false);

    // Rules determine all bays, windows, and terminal geometry.
    GenerateTree(&root);
    AddRoot(std::move(root));
}

void GrammarEngine::AssignBuildingId(ShapeNode& node, int buildingId) {
    node.buildingId = buildingId;
    for (ShapeNode& child : node.children)
        AssignBuildingId(child, buildingId);
}

int GrammarEngine::AddRoot(ShapeNode root) {
    const int buildingId = nextBuildingId++;
    AssignBuildingId(root, buildingId);
    roots.push_back(std::move(root));
    RebuildTerminals();
    return buildingId;
}

const ShapeNode* GrammarEngine::FindRoot(int buildingId) const {
    for (const ShapeNode& root : roots)
        if (root.buildingId == buildingId) return &root;
    return nullptr;
}

void GrammarEngine::ReplaceRoot(int buildingId, ShapeNode root) {
    for (ShapeNode& current : roots) {
        if (current.buildingId != buildingId) continue;
        AssignBuildingId(root, buildingId);
        current = std::move(root);
        RebuildTerminals();
        return;
    }
    throw std::invalid_argument("Building no longer exists.");
}

void GrammarEngine::MoveRoot(int buildingId, V3 position) {
    for (ShapeNode& root : roots) {
        if (root.buildingId != buildingId) continue;
        root.position = position;
        RebuildTerminals();
        return;
    }
    throw std::invalid_argument("Building no longer exists.");
}

std::vector<ShapeNode> GrammarEngine::TerminalsForRoot(const ShapeNode& root) const {
    std::vector<ShapeNode> result;
    CollectInto(&root, V3(0, 0, 0), M33(1.0f), root.buildingId, result);
    return result;
}

void GrammarEngine::RebuildTerminals() {
    std::vector<ShapeNode> result;
    for (const ShapeNode& root : roots)
        CollectInto(&root, V3(0, 0, 0), M33(1.0f), root.buildingId, result);
    terminals.swap(result);
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
    building_rules::Expand(node);

    // recursively generate grammar tree for each child
    for (ShapeNode& child : node->children) {
        GenerateTree(&child);
    }
}

// collect the terminal nodes with their world pose
void GrammarEngine::CollectTerminals(ShapeNode* node, V3 parentPose, M33 parentRotation) {
    CollectInto(node, parentPose, parentRotation, node ? node->buildingId : -1, terminals);
}

void GrammarEngine::CollectInto(const ShapeNode* node, V3 parentPose, M33 parentRotation,
                                int buildingId, std::vector<ShapeNode>& output) {
    if (node == nullptr) {
        std::cerr << "Error: CollectTerminals called with null node." << std::endl;
        return;
    }

    V3 newParentPose = parentPose + parentRotation * node->position;
    M33 worldRotation = parentRotation * node->rotation;

    // If the node is terminal, add it to the terminals vector
    if (node->isTerminal) {
        // copy and transform to world pose first
        ShapeNode terminal = *node;
        terminal.position = newParentPose;
        terminal.rotation = worldRotation;
        terminal.buildingId = buildingId;
        output.push_back(std::move(terminal));
        return;
    }

    // If the node is not terminal, recursively collect terminals from its children
    for (const ShapeNode& child : node->children) {
        CollectInto(&child, newParentPose, worldRotation, buildingId, output);
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
