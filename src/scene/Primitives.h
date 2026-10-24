// Primitives.h
//Sydney Vitalbo

#ifndef SCENE_PRIMITIVES_H
#define SCENE_PRIMITIVES_H

#include "scene/Mesh.h"

namespace sg {

namespace primitives {

/**
 * unit box centered at the origin.
 *
 * Spans [-0.5, +0.5] on each of x, y, z (edge length 1)
 */
Mesh makeUnitBox();

/**
 * unit cylinder centered at the origin, aligned to the +Y axis
 *
 * Radius 0.5, height 1 (spans y in [-0.5, +0.5])
 * segments: more means smoother
 */
Mesh makeUnitCylinder(int segments = 24);

/**
 * flat reference grid on the ground plane (y = 0)
 */
struct GroundGrid {
    float halfExtent = 10.0f; // grid spans [-halfExtent, +halfExtent]
    float spacing = 1.0f;     // distance between grid lines (the snap unit)
};

} // namespace primitives
} // namespace sg

#endif // SCENE_PRIMITIVES_H
