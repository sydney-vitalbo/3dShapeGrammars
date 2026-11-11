#include "grammar/BuildingRules.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

using namespace grammar_ops;

namespace building_rules {

// Preserve a box's exterior using independently disableable wall pieces.
// These are immediate terminal children; no rule expansion happens here.
static std::vector<ShapeNode> shell(const ShapeNode& parent, ShapeNode scope,
                                     Material material, bool groundBottom = false) {
    const float w = scope.size[0], h = scope.size[1], d = scope.size[2];
    const float t = std::min(0.1f, std::min(w, std::min(h,d))*0.25f);
    std::vector<ShapeNode> pieces{
        terminal(parent,material,V3(w,h,t),scope.position+V3(0,0,(d-t)/2)),
        terminal(parent,material,V3(w,h,t),scope.position+V3(0,0,-(d-t)/2)),
        terminal(parent,material,V3(t,h,d-2*t),scope.position+V3((w-t)/2,0,0)),
        terminal(parent,material,V3(t,h,d-2*t),scope.position+V3(-(w-t)/2,0,0)),
        terminal(parent,material,V3(w-2*t,t,d-2*t),scope.position+V3(0,(h-t)/2,0)),
        terminal(parent,material,V3(w-2*t,t,d-2*t),scope.position+V3(0,-(h-t)/2,0))
    };
    pieces[0].exteriorFace = BuildingFace::Front;
    pieces[1].exteriorFace = BuildingFace::Back;
    pieces[2].exteriorFace = BuildingFace::Right;
    pieces[3].exteriorFace = BuildingFace::Left;
    if (groundBottom) pieces[5].exteriorFace = BuildingFace::Bottom;
    return pieces;
}

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
    regions[1].exteriorFace = BuildingFace::Top;
    children.push_back(std::move(regions[1]));
    node->children = std::move(children);
}

// Floor -> wall-body shell + front/rear WallRegions + floor-band shell.
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
    front.exteriorFace = BuildingFace::Front;
    rear.exteriorFace = BuildingFace::Back;
    if (!rear.building.streetFacing)
        transform(rear, V3(0, 0, 0), M33(V3(0, 1, 0), 180.0f));

    const Material wallMaterial = node->building.apartment ? Material::Stone : Material::WhiteMarble;
    auto children = shell(*node, wall, wallMaterial, node->building.groundFloor);
    children.push_back(std::move(front));
    children.push_back(std::move(rear));
    auto band = shell(*node, regions[1], node->building.apartment ? Material::RedMarble : Material::Stone);
    for (ShapeNode& piece : band) children.push_back(std::move(piece));
    node->children = std::move(children);
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
        terminal(*node, Material::RedMarble,
            V3(1.12f * node->size[0], 0.07f, canopyDepth),
            V3(0, 0.45f * node->size[1], canopyDepth / 2))
    };
}

// Window -> top/side frame pieces + center mullion + sill + two inset panes.
// The panes fill separate openings; no solid backing box or glass under the
// mullion. Frame pieces meet at edges instead of sharing visible face areas.
void expandWindow(ShapeNode* node) {
    const Material frame = node->building.apartment ? Material::WhiteMarble : Material::Stone;
    const float w = node->size[0], h = node->size[1], d = node->size[2];
    const float side = 0.06f * w, trim = 0.065f * h;
    const float openingHeight = h - 2 * trim, mullion = 0.035f * w;
    const float paneWidth = (w - 2 * side - mullion) / 2;
    const float paneX = (mullion + paneWidth) / 2;
    node->children = {
        terminal(*node, frame, V3(w, trim, d), V3(0, (h - trim) / 2, 0)),
        terminal(*node, frame, V3(side, openingHeight, d), V3(-(w - side) / 2, 0, 0)),
        terminal(*node, frame, V3(side, openingHeight, d), V3((w - side) / 2, 0, 0)),
        terminal(*node, frame, V3(mullion, openingHeight, d)),
        // The sill replaces the bottom frame piece. Its back aligns with the
        // frame and its bottom stays inside the window (also used for doors).
        terminal(*node, frame, V3(1.06f * w, trim, 1.35f * d),
            V3(0, -(h - trim) / 2, 0.175f * d)),
        terminal(*node, Material::Glass, V3(paneWidth, openingHeight, 0.35f * d),
            V3(-paneX, 0, 0.225f * d)),
        terminal(*node, Material::Glass, V3(paneWidth, openingHeight, 0.35f * d),
            V3(paneX, 0, 0.225f * d))
    };
}

// Balcony -> terminal floor slab + low front wall + two side walls.
void expandBalcony(ShapeNode* node) {
    constexpr float slab = 0.08f, rail = 0.055f, guard = 0.43f;
    const float w = node->size[0], h = node->size[1], d = node->size[2];
    const float guardY = -h / 2 + slab + guard / 2;
    // Side guards stop at the front guard's inner face, avoiding overlapping
    // top faces at the corners. All guards stand on, not inside, the slab.
    node->children = {
        terminal(*node, Material::Stone, V3(w, slab, d), V3(0, -h / 2 + slab / 2, 0)),
        terminal(*node, Material::Wood, V3(w, guard, rail), V3(0, guardY, (d - rail) / 2)),
        terminal(*node, Material::Wood, V3(rail, guard, d - rail), V3(-(w - rail) / 2, guardY, -rail / 2)),
        terminal(*node, Material::Wood, V3(rail, guard, d - rail), V3((w - rail) / 2, guardY, -rail / 2))
    };
}

// FlatRoof -> terminal roof deck + four parapet walls.
void expandRoof(ShapeNode* node) {
    constexpr float thickness = 0.10f;
    const float w = node->size[0], h = node->size[1], d = node->size[2];
    const Material edge = node->building.apartment ? Material::RedMarble : Material::Stone;
    // The deck fills the footprint and parapets sit on it. Removing an internal
    // parapet during joining therefore leaves a continuous roof surface.
    const float deckHeight = 0.25f*h, parapetHeight = h-deckHeight;
    node->children = {
        terminal(*node, Material::None, V3(w, deckHeight, d),
            V3(0, -0.375f * h, 0)),
        terminal(*node, edge, V3(w, parapetHeight, thickness), V3(0, deckHeight/2, -(d-thickness)/2)),
        terminal(*node, edge, V3(w, parapetHeight, thickness), V3(0, deckHeight/2, (d-thickness)/2)),
        terminal(*node, edge, V3(thickness, parapetHeight, d-2*thickness), V3(-(w-thickness)/2, deckHeight/2, 0)),
        terminal(*node, edge, V3(thickness, parapetHeight, d-2*thickness), V3((w-thickness)/2, deckHeight/2, 0))
    };
    node->children[1].exteriorFace = BuildingFace::Back;
    node->children[2].exteriorFace = BuildingFace::Front;
    node->children[3].exteriorFace = BuildingFace::Left;
    node->children[4].exteriorFace = BuildingFace::Right;
}

void Expand(ShapeNode* node) {
    if (!node || node->isTerminal)
        return;

    switch (node->symbol) {
    case Symbol::Office:
    case Symbol::Apartment:
        expandBuilding(node);
        break;
    case Symbol::Floor:
        expandFloor(node);
        break;
    case Symbol::WallRegion:
        expandWallRegion(node);
        break;
    case Symbol::BayGroup:
        expandBayGroup(node);
        break;
    case Symbol::Bay:
        expandBay(node);
        break;
    case Symbol::Entrance:
        expandEntrance(node);
        break;
    case Symbol::Balcony:
        expandBalcony(node);
        break;
    case Symbol::Roof:
        expandRoof(node);
        break;
    case Symbol::Window:
        expandWindow(node);
        break;
    case Symbol::Facade:
        expandFacade(node);
        break;
    default:
        break;
    }
}

} // namespace building_rules
