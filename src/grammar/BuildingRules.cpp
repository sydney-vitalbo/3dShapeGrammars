#include "grammar/BuildingRules.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

using namespace grammar_ops;

namespace building_rules {

void validate(const Parameters& p) {
    if (p.floors < 1 || p.floors > 12 || p.bays < 2 || p.bays > 8 ||
        !std::isfinite(p.width) || p.width < 4 || p.width > 12 ||
        !std::isfinite(p.floorHeight) || p.floorHeight < 1.2f || p.floorHeight > 3 ||
        !std::isfinite(p.depth) || p.depth < 2 || p.depth > 8)
        throw std::invalid_argument("Use 1-12 floors, 2-8 bays, width 4-12, floor height 1.2-3, depth 2-8.");
}

ShapeNode buildingRoot(const Parameters& p, bool apartment) {
    building_rules::validate(p);
    ShapeNode root;
    root.symbol = apartment ? Symbol::Apartment : Symbol::Office;
    const float height = p.floors * p.floorHeight + roofHeight;
    root.size = V3(p.width, height, p.depth);
    root.position = V3(0, height / 2, 0); // Base remains at ground level.
    root.building.floors = p.floors;
    root.building.bays = p.bays;
    root.building.apartment = apartment;
    root.building.balconies = p.balconies;
    return root;
}

ShapeNode makeOffice(const Parameters& p) { return buildingRoot(p, false); }
ShapeNode makeApartment(const Parameters& p) { return buildingRoot(p, true); }

// Office/Apartment -> equal Floor regions + FlatRoof. Children are unexpanded.
void expandBuilding(ShapeNode* node) {
    const int count = node->building.floors;
    if (count < 1 || !std::isfinite(node->size[1]) || node->size[1] <= roofHeight)
        throw std::invalid_argument("Building needs a positive floor count and space below the roof.");
    node->building.apartment = node->symbol == Symbol::Apartment;
    const float floorHeight = (node->size[1] - roofHeight) / count;
    if (floorHeight <= bandHeight)
        throw std::invalid_argument("Floor height must exceed the floor band height.");

    auto regions = subdivide(*node, Axis::Y,
        {Split::relative(1), Split::fixed(roofHeight)});
    auto children = repeat(regions[0], Axis::Y, count);
    for (int i = 0; i < count; ++i) {
        children[i].symbol = Symbol::Floor;
        children[i].building.groundFloor = i == 0;
        // Flatten the temporary floor region: floors are direct children of
        // the building, so their positions must be local to the building.
        children[i].position = regions[0].position + children[i].position;
    }
    regions[1].symbol = Symbol::Roof;
    children.push_back(std::move(regions[1]));
    node->children = std::move(children);
}

// Floor -> solid wall body + front/rear WallRegions + terminal FloorBand.
void expandFloor(ShapeNode* node) {
    auto regions = subdivide(*node, Axis::Y,
        {Split::relative(1), Split::fixed(bandHeight)});
    ShapeNode& wall = regions[0];
    constexpr float facadeDepth = 0.2f;
    const V3 facadeSize(wall.size[0], wall.size[1], facadeDepth);
    ShapeNode front = childOf(*node, Symbol::WallRegion, facadeSize,
        wall.position + V3(0, 0, wall.size[2] / 2));
    ShapeNode rear = childOf(*node, Symbol::WallRegion, facadeSize,
        wall.position + V3(0, 0, -wall.size[2] / 2));
    front.building.streetFacing = true;
    rear.building.streetFacing = false;
    if (!rear.building.streetFacing)
        transform(rear, V3(0, 0, 0), M33(V3(0, 1, 0), 180.0f));

    regions[1].isTerminal = true;
    regions[1].material = node->building.apartment ? Material::Stone : Material::Wood;
    node->children = {
        terminal(*node, Material::Stone, wall.size, wall.position),
        std::move(front), std::move(rear), std::move(regions[1])
    };
}

// Each WallRegion is already placed/oriented at one wall. Its local +Z points
// outward, so the same facade production works on both sides of the building.
void expandWallRegion(ShapeNode* node) {
    node->children = {childOf(*node, Symbol::Facade, node->size)};
}

// Ground street facade -> left BayGroup + Entrance + right BayGroup.
// Other facades -> repeated Bay scopes. No bays are expanded here.
void expandFacade(ShapeNode* node) {
    if (node->building.bays < 1)
        throw std::invalid_argument("Facade needs at least one bay.");
    std::vector<ShapeNode> children;
    if (node->building.groundFloor && node->building.streetFacing) {
        const float doorWidth = node->building.apartment ? 1.0f : 1.5f;
        children = subdivide(*node, Axis::X,
            {Split::relative(1), Split::fixed(doorWidth), Split::relative(1)});
        children[0].symbol = Symbol::BayGroup;
        children[1].symbol = Symbol::Entrance;
        children[2].symbol = Symbol::BayGroup;
    } else {
        const int count = node->building.bays;
        children = repeat(*node, Axis::X, count);
        for (ShapeNode& child : children)
            child.symbol = Symbol::Bay;
    }
    node->children = std::move(children);
}

// Side of a ground-floor entrance -> repeated, unexpanded Bay scopes.
void expandBayGroup(ShapeNode* node) {
    const int count = std::max(1, node->building.bays / 2);
    auto children = repeat(*node, Axis::X, count);
    for (ShapeNode& child : children)
        child.symbol = Symbol::Bay;
    node->children = std::move(children);
}

// Bay -> Window, optionally accompanied by an unexpanded Balcony.
void expandBay(ShapeNode* node) {
    const bool withBalcony = node->building.apartment && !node->building.groundFloor
        && node->building.streetFacing && node->building.balconies;
    ShapeNode pane = childOf(*node, Symbol::Window,
        V3((node->building.apartment ? 0.60f : 0.90f) * node->size[0],
           (withBalcony ? 0.86f : 0.76f) * node->size[1], 0.065f),
        V3(0, (withBalcony ? -0.01f : 0.02f) * node->size[1],
           0.03f)); // Small outward offset from the wall surface.
    std::vector<ShapeNode> children{std::move(pane)};
    if (withBalcony) {
        constexpr float projection = 0.65f;
        children.push_back(childOf(*node, Symbol::Balcony,
            V3(node->size[0] * 0.82f, node->size[1], projection),
            V3(0, 0, projection / 2)));
    }
    node->children = std::move(children);
}

// Entrance -> unexpanded door Window + terminal canopy.
void expandEntrance(ShapeNode* node) {
    const float canopyDepth = node->building.apartment ? 0.38f : 0.55f;
    node->children = {
        childOf(*node, Symbol::Window,
            V3(0.88f * node->size[0], 0.91f * node->size[1], 0.08f),
            V3(0, -0.045f * node->size[1], 0.04f)),
        terminal(*node, node->building.apartment ? Material::Stone : Material::Wood,
            V3(1.12f * node->size[0], 0.07f, canopyDepth),
            V3(0, 0.45f * node->size[1], canopyDepth / 2))
    };
}

// Window -> terminal frame + glass + center mullion + sill.
void expandWindow(ShapeNode* node) {
    const Material frame = node->building.apartment ? Material::Stone : Material::Wood;
    const float w = node->size[0], h = node->size[1], d = node->size[2];
    node->children = {
        terminal(*node, frame, node->size),
        terminal(*node, Material::Glass, V3(0.88f * w, 0.87f * h, 0.35f * d),
            V3(0, 0, 0.55f * d)),
        terminal(*node, frame, V3(0.035f * w, 0.87f * h, 0.36f * d),
            V3(0, 0, 0.56f * d)),
        terminal(*node, frame, V3(1.06f * w, 0.055f * h, 1.5f * d),
            V3(0, -0.48f * h, 0.1f * d))
    };
}

// Balcony -> terminal floor slab + low front wall + two side walls.
void expandBalcony(ShapeNode* node) {
    constexpr float slab = 0.08f, rail = 0.055f, guard = 0.43f;
    const float w = node->size[0], h = node->size[1], d = node->size[2];
    const float guardY = -h / 2 + slab + guard / 2;
    node->children = {
        terminal(*node, Material::Stone, V3(w, slab, d), V3(0, -h / 2 + slab / 2, 0)),
        terminal(*node, Material::Wood, V3(w, guard, rail), V3(0, guardY, (d - rail) / 2)),
        terminal(*node, Material::Wood, V3(rail, guard, d), V3(-(w - rail) / 2, guardY, 0)),
        terminal(*node, Material::Wood, V3(rail, guard, d), V3((w - rail) / 2, guardY, 0))
    };
}

// FlatRoof -> terminal roof deck + four parapet walls.
void expandRoof(ShapeNode* node) {
    constexpr float thickness = 0.10f;
    const float w = node->size[0], h = node->size[1], d = node->size[2];
    const Material edge = node->building.apartment ? Material::Wood : Material::Stone;
    node->children = {
        terminal(*node, Material::None, V3(w, 0.25f * h, d), V3(0, -0.375f * h, 0)),
        terminal(*node, edge, V3(w, h, thickness), V3(0, 0, -(d - thickness) / 2)),
        terminal(*node, edge, V3(w, h, thickness), V3(0, 0, (d - thickness) / 2)),
        terminal(*node, edge, V3(thickness, h, d), V3(-(w - thickness) / 2, 0, 0)),
        terminal(*node, edge, V3(thickness, h, d), V3((w - thickness) / 2, 0, 0))
    };
}
} // namespace building_rules
