#include "grammar/BuildingRules.h"
#include "grammar/Operations.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace building_rules {
namespace {
using namespace grammar_ops;
constexpr float bandHeight = 0.12f;
constexpr float roofHeight = 0.24f;

// The names in these production comments describe the authored rules; they do
// not add symbols to the team's existing Symbol enum. All leaves remain boxes.
// Rules depend on the Operations.h interface; implementations belong to the engine.

// Window -> frame + glass + center mullion + sill.
// front = +1 for the street facade, -1 for the rear facade.
ShapeNode window(bool apartment, float front) {
    const Material frame = apartment ? Material::Stone : Material::Wood;
    std::vector<ShapeNode> parts{
        terminal(frame),
        terminal(Material::Glass, V3(0.88f, 0.87f, 0.35f), V3(0, 0, front * 0.55f)),
        terminal(frame, V3(0.035f, 0.87f, 0.36f), V3(0, 0, front * 0.56f)),
        terminal(frame, V3(1.06f, 0.055f, 1.5f), V3(0, -0.48f, front * 0.1f))
    };
    ShapeNode scope;
    scope.symbol = Symbol::Window;
    return compose(scope, std::move(parts));
}

// Balcony -> floor slab + low front wall + left wall + right wall.
// The open back meets the building. No rail meshes, supports, or interiors.
ShapeNode balcony(float bayWidth, float wallHeight, float depth) {
    constexpr float projection = 0.65f, slab = 0.08f, rail = 0.055f, guard = 0.43f;
    const float width = bayWidth * 0.82f;
    const float bottom = -0.5f + slab / (2 * wallHeight);
    const float guardY = -0.5f + (slab + guard / 2) / wallHeight;
    std::vector<ShapeNode> parts{
        terminal(Material::Stone, V3(1, slab / wallHeight, 1), V3(0, bottom, 0)),
        terminal(Material::Wood, V3(1, guard / wallHeight, rail / projection),
                 V3(0, guardY, 0.5f - rail / (2 * projection))),
        terminal(Material::Wood, V3(rail / width, guard / wallHeight, 1),
                 V3(-0.5f + rail / (2 * width), guardY, 0)),
        terminal(Material::Wood, V3(rail / width, guard / wallHeight, 1),
                 V3(0.5f - rail / (2 * width), guardY, 0))
    };
    ShapeNode scope;
    scope.size = V3(0.82f, 1, projection / depth);
    scope.position = V3(0, 0, 0.5f + projection / (2 * depth));
    return compose(scope, std::move(parts));
}

// OfficeBay -> wide framed glass panel.
// ApartmentBay -> smaller framed window
//              | balcony door + Balcony (only when the user enables balconies).
ShapeNode bay(ShapeNode scope, const Parameters& p, float bayWidth,
              bool apartment, bool withBalcony, float front) {
    const float wallHeight = p.floorHeight - bandHeight;
    ShapeNode pane = window(apartment, front);
    pane.size = V3(apartment ? 0.60f : 0.90f, withBalcony ? 0.86f : 0.76f,
                   0.065f / p.depth);
    pane.position = V3(0, withBalcony ? -0.01f : 0.02f, front * 0.505f);
    std::vector<ShapeNode> parts{std::move(pane)};
    if (withBalcony) parts.push_back(balcony(bayWidth, wallHeight, p.depth));
    return compose(std::move(scope), std::move(parts));
}

// Entrance -> framed glass door + shallow canopy.
// Office entrances are wider; both doors reach the ground instead of floating.
ShapeNode entrance(ShapeNode scope, const Parameters& p, bool apartment) {
    const float wallHeight = p.floorHeight - bandHeight;
    ShapeNode door = window(apartment, 1);
    door.size = V3(0.88f, 0.91f, 0.08f / p.depth);
    door.position = V3(0, -0.045f, 0.507f);
    const float canopyDepth = apartment ? 0.38f : 0.55f;
    ShapeNode canopy = terminal(apartment ? Material::Stone : Material::Wood,
        V3(1.12f, 0.07f / wallHeight, canopyDepth / p.depth),
        V3(0, 0.45f, 0.5f + canopyDepth / (2 * p.depth)));
    return compose(std::move(scope), {std::move(door), std::move(canopy)});
}

// GroundFacade -> repeated left windows + Entrance + repeated right windows.
// UpperOfficeFacade -> repeat_X(OfficeBay).
// UpperApartmentFacade -> repeat_X(ApartmentBay).
ShapeNode facade(const Parameters& p, bool apartment, bool ground, float front) {
    std::vector<ShapeNode> bays;
    if (ground && front > 0) {
        const float doorWidth = apartment ? 1.0f : 1.5f;
        bays = subdivide(Axis::X, p.width,
                        {Split::relative(1), Split::fixed(doorWidth), Split::relative(1)});
        const int count = std::max(1, p.bays / 2);
        for (int side : {0, 2}) {
            auto windows = repeat(Axis::X, count);
            for (ShapeNode& item : windows)
                item = bay(std::move(item), p, (p.width - doorWidth) / (2 * count),
                           apartment, false, front);
            bays[side] = compose(std::move(bays[side]), std::move(windows));
        }
        bays[1] = entrance(std::move(bays[1]), p, apartment);
    } else {
        bays = repeat(Axis::X, p.bays);
        for (ShapeNode& item : bays)
            item = bay(std::move(item), p, p.width / p.bays, apartment,
                       apartment && !ground && front > 0 && p.balconies, front);
    }
    return compose(ShapeNode{}, std::move(bays));
}

// Floor -> subdivide_Y(WallRegion: remaining height, FloorBand: fixed 0.12).
// WallRegion -> solid wall + front facade + rear facade.
// Side walls stay plain for neighboring buildings; no joining/booleans here.
ShapeNode floor(ShapeNode scope, const Parameters& p, bool apartment, bool ground) {
    auto regions = subdivide(Axis::Y, p.floorHeight,
                             {Split::relative(1), Split::fixed(bandHeight)});
    regions[0] = compose(std::move(regions[0]), {
        terminal(Material::Stone), facade(p, apartment, ground, 1),
        facade(p, apartment, ground, -1)
    });
    regions[1].isTerminal = true;
    regions[1].material = apartment ? Material::Stone : Material::Wood;
    return compose(std::move(scope), std::move(regions));
}

// FlatRoof -> inset roof deck + four thin parapet walls.
// All roof edges stay inside the footprint so neighboring roofs do not overlap.
ShapeNode roof(ShapeNode scope, const Parameters& p, bool apartment) {
    constexpr float thickness = 0.10f;
    const Material edge = apartment ? Material::Wood : Material::Stone;
    return compose(std::move(scope), {
        terminal(Material::None, V3(1, 0.25f, 1), V3(0, -0.375f, 0)),
        terminal(edge, V3(1, 1, thickness / p.depth), V3(0, 0, -0.5f + thickness / (2 * p.depth))),
        terminal(edge, V3(1, 1, thickness / p.depth), V3(0, 0, 0.5f - thickness / (2 * p.depth))),
        terminal(edge, V3(thickness / p.width, 1, 1), V3(-0.5f + thickness / (2 * p.width), 0, 0)),
        terminal(edge, V3(thickness / p.width, 1, 1), V3(0.5f - thickness / (2 * p.width), 0, 0))
    });
}

// Office/Apartment -> GroundFloor + repeat_Y(UpperFloor, floors - 1) + FlatRoof.
// Implemented as equal floor scopes with a special ground-floor production.
// Changing the count repeats floors; it does not stretch the floor height.
ShapeNode building(const Parameters& p, bool apartment) {
    if (p.floors < 1 || p.floors > 12 || p.bays < 2 || p.bays > 8 ||
        !std::isfinite(p.width) || p.width < 4 || p.width > 12 ||
        !std::isfinite(p.floorHeight) || p.floorHeight < 1.2f || p.floorHeight > 3 ||
        !std::isfinite(p.depth) || p.depth < 2 || p.depth > 8)
        throw std::invalid_argument("Use 1-12 floors, 2-8 bays, width 4-12, floor height 1.2-3, depth 2-8.");
    ShapeNode root;
    root.symbol = Symbol::Facade;
    const float height = p.floors * p.floorHeight + roofHeight;
    root.size = V3(p.width, height, p.depth);
    root.position = V3(0, height / 2, 0); // Base remains at ground level.
    auto sections = subdivide(Axis::Y, height,
                              {Split::fixed(p.floors * p.floorHeight), Split::fixed(roofHeight)});
    auto floors = repeat(Axis::Y, p.floors);
    for (int i = 0; i < p.floors; ++i)
        floors[i] = floor(std::move(floors[i]), p, apartment, i == 0);
    sections[0] = compose(std::move(sections[0]), std::move(floors));
    sections[1] = roof(std::move(sections[1]), p, apartment);
    return compose(std::move(root), std::move(sections));
}
} // namespace

// Office -> wide glass bays, broad entrance, dark horizontal floor bands.
ShapeNode makeOffice(const Parameters& p) { return building(p, false); }

// Apartment -> smaller windows, residential entrance, optional front balconies.
ShapeNode makeApartment(const Parameters& p) { return building(p, true); }

} // namespace building_rules
