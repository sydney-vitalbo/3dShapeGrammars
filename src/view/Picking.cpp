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

//Determine if the ray hits this box, via the "slab method"
// Returns true on a hit and writes the entry distance to outT (used to pick the closest terminal)
bool rayHitBox(const V3& origin, const V3& dir,
               const V3& center, const V3& size, const M33& rotation,
               float& outT) {
    //V3/M33 math API has no const methods, so copy into mutable locals
    V3 org = origin, direction = dir;
    V3 c = center;
    V3 s = size;
    M33 rot = rotation;
    M33 inv = rot.Transpose(); // world-orientation -> local-orientation (inverse)

    // Move the ray into the box's local space: subtract the box center (so the
    // box sits at the origin) and un-rotate it. Now the box is axis-aligned and
    // spans [-half, +half] on each axis
    V3 o = inv * (org - c);      // ray start,  in local space
    V3 d = inv * direction;      // ray heading, in local space
    V3 half = s * 0.5f;          // half the box size on each axis

    //start with "the whole ray" and shrink the valid [tMin, tMax] window one axis at a time
    float tMin = 0.0f;
    float tMax = std::numeric_limits<float>::max();
    for (int i = 0; i < 3; ++i) { // i = 0,1,2  ->  X, Y, Z axis
        const float oi = o[i], di = d[i], h = half[i];
        if (std::abs(di) < 1e-8f) {
            //ray runs parallel to this pair of walls (it never crosses them)
            //then it can only be inside if it already STARTS between them
            if (oi < -h || oi > h) return false; // started outside -> definite miss
        } else {
            //distances at which the ray crosses this axis's two walls (-h and +h)
            float t1 = (-h - oi) / di;
            float t2 = ( h - oi) / di;
            if (t1 > t2) std::swap(t1, t2); // make t1 the near wall, t2 the far wall
            //tighten the overlap window to this slab's [t1, t2].
            tMin = std::max(tMin, t1); // latest entry so far
            tMax = std::min(tMax, t2); // earliest exit so far
            if (tMin > tMax) return false; // slabs don't overlap -> the ray misses
        }
    }
    //survived all three axes: the ray enters the box at distance tMin
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
