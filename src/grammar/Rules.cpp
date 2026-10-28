#include "Rules.h"

// mridu's hardcoded testing rules for facade and window expansion
void Rules::Window(ShapeNode* parent) {

    // Create four terminal Glass children
    for (int i = 0; i < 4; ++i) {
        ShapeNode child;
        child.id = parent->id + i + 1; // Unique ID based on parent
        child.symbol = Symbol::Slab;
        child.position = V3((i % 2) - 0.5f, (i / 2) - 0.5f, 0.0f);
        child.size = V3(0.95f, 0.95f, 0.05f);
        child.isTerminal = true;
        child.material = Material::Glass;

        parent->children.push_back(child);
    }
}

void Rules::Facade(ShapeNode* parent) {
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
        child.id = parent->id + i + 1; // Unique ID based on parent
        child.symbol = Symbol::Slab;
        child.position = woodPositions[i];
        child.size = i < 2 ? V3(1.0f, 4.0f, 0.2f) : V3(2.0f, 1.0f, 0.2f);
        child.isTerminal = true;
        child.material = i < 2 ? Material::Stone : Material::Wood;
        parent->children.push_back(child);
    }

    // Create non terminal Window child
    ShapeNode windowChild;
    windowChild.id = parent->id + 5; // Unique ID based on parent
    windowChild.symbol = Symbol::Window;
    windowChild.position = V3(0.0f, 0.0f, 0.0f); // Middle of facade
    windowChild.size = V3(2.0f, 2.0f, 0.2f);
    windowChild.isTerminal = false;

    parent->children.push_back(windowChild);
}

void Rules::Expand(ShapeNode* node) {
    if (!node || node->isTerminal)
        return;

    switch (node->symbol) {
        case Symbol::Office:
        case Symbol::Apartment:
            building_rules::expandBuilding(node);
            break;
        case Symbol::Floor:
            building_rules::expandFloor(node);
            break;
        case Symbol::WallRegion:
            building_rules::expandWallRegion(node);
            break;
        case Symbol::BayGroup:
            building_rules::expandBayGroup(node);
            break;
        case Symbol::Bay:
            building_rules::expandBay(node);
            break;
        case Symbol::Entrance:
            building_rules::expandEntrance(node);
            break;
        case Symbol::Balcony:
            building_rules::expandBalcony(node);
            break;
        case Symbol::Roof:
            building_rules::expandRoof(node);
            break;
        case Symbol::Window:
            building_rules::expandWindow(node);
            break;

        case Symbol::Facade:
            building_rules::expandFacade(node);
            break;

        default:
            break;
    }
}
