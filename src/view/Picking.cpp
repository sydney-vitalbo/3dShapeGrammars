// Picking.cpp
// Pure ray-geometry for selection and dragging. No OpenGL / FLTK here.
// Extracted from SceneView so this math can be read and tested on its own.

#include "view/Picking.h"

#include <algorithm> // std::min, std::max
#include <cmath>     // std::abs
#include <limits>    // std::numeric_limits
#include <utility>   // std::swap

namespace sg {
namespace picking {

// Ray vs. axis-aligned box, via the "slab method".
// Buildings are axis-aligned in world space, so this is the exact test used to
// pick the nearest building; outT is the entry distance.
bool rayHitAABB(const V3& origin, const V3& dir,
                const V3& minC, const V3& maxC,
                float& outT) {
    // V3's subscript operator is non-const, so copy into mutable locals.
    V3 o = origin, d = dir;
    V3 lo = minC, hi = maxC;

    // Start with "the whole ray" and shrink the valid [tMin, tMax] window one
    // axis at a time.
    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::max();
    for (int i = 0; i < 3; ++i) { // i = 0,1,2 -> X, Y, Z axis
        const float oi = o[i], di = d[i];
        if (std::abs(di) < 1e-8f) {
            // Ray runs parallel to this pair of walls: it can only be inside if
            // it already STARTS between them.
            if (oi < lo[i] || oi > hi[i]) return false; // started outside -> miss
        } else {
            // Distances at which the ray crosses this axis's two walls.
            float t1 = (lo[i] - oi) / di;
            float t2 = (hi[i] - oi) / di;
            if (t1 > t2) std::swap(t1, t2); // t1 = near wall, t2 = far wall
            tMin = std::max(tMin, t1); // latest entry so far
            tMax = std::min(tMax, t2); // earliest exit so far
            if (tMin > tMax) return false; // slabs don't overlap -> miss
        }
    }
    // Survived all three axes: the ray enters the box at distance tMin
    // (tMin is 0 when the ray's origin is already inside the box).
    outT = tMin;
    return true;
}

// Find where a ray crosses the flat ground plane (y = 0)
//
// just need the t where the ray's y-component becomes 0:
//     origin.y + t * dir.y = 0   ->   t = -origin.y / dir.y
// If dir.y is ~0 the ray runs parallel to the ground, if t is negative the ground is behind the camera (no hit)
bool rayHitGround(const V3& origin, const V3& dir, V3& outPoint) {
    V3 o = origin, d = dir; // local mutable copies (V3 API is non-const)
    const float dy = d[1];
    if (std::abs(dy) < 1e-8f) return false;     // parallel to the ground
    const float t = -o[1] / dy;
    if (t < 0.0f) return false;                 // ground is behind the camera
    outPoint = o + d * t;                       // the point where y == 0
    return true;
}

} // namespace picking
} // namespace sg
