#include "Rules.h"

std::vector<ShapeNode> Rules::Window(const ShapeNode parent) {
    std::vector<ShapeNode> children;

    // Create four terminal Glass children
    for (int i = 0; i < 4; ++i) {
        ShapeNode child;
        child.id = parent.id + i; // Unique ID based on parent
        child.symbol = Symbol::Slab;
        child.position = V3((i % 2) - 0.5f, (i / 2) - 0.5f, 0.0f);
        child.size = V3(1.0f, 1.0f, 0.1f) * 0.95f;
        child.isTerminal = true;
        child.material = Material::Glass;

        children.push_back(child);
    }

    return children;
}

std::vector<ShapeNode> Rules::Facade(const ShapeNode parent) {
    std::vector<ShapeNode> children;

    const V3 woodPositions[4] = {
        V3(-1.5f, 0.0f, 0.0f), // Left
        V3(1.5f, 0.0f, 0.0f),  // Right
        V3(0.0f, 1.5f, 0.0f),  // Top
        V3(0.0f, -1.5f, 0.0f)  // Bottom
    };

    // Create four terminal Wood children
    for (int i = 0; i < 4; ++i) {
        ShapeNode child;
        child.id = parent.id + i; // Unique ID based on parent
        child.symbol = Symbol::Slab;
        child.position = woodPositions[i];
        child.size = i < 2 ? V3(1.0f, 4.0f, 1.0f) : V3(2.0f, 1.0f, 1.0f);
        child.isTerminal = true;
        child.material = i < 2 ? Material::Stone : Material::Wood;

        children.push_back(child);
    }

    // Create non terminal Window child
    ShapeNode windowChild;
    windowChild.id = parent.id + 4; // Unique ID based on parent
    windowChild.symbol = Symbol::Window;
    windowChild.position = V3(0.0f, 0.0f, 0.0f); // Middle of facade
    windowChild.size = V3(1.0f, 1.0f, 0.1f);
    windowChild.isTerminal = false;

    children.push_back(windowChild);

    return children;
}

std::vector<ShapeNode> Rules::Expand(const ShapeNode node) {
    switch (node.symbol) {
        case Symbol::Window:
            return Window(node); // Four Glass children

        case Symbol::Facade:
            return Facade(node); // Four Wood children and one Window child

        default:
            return {};
    }
}
