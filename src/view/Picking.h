// Picking.h: geometry behind mouse picking.
//
// These two functions are the testable heart of selection and dragging: given a
// ray (built elsewhere from the camera + cursor), they answer "does it hit this
// box?" and "where does it meet the ground?". They touch no OpenGL, FLTK, or
// view state, so they can be exercised in isolation with plain V3/M33 values.
//
// (Building the ray itself lives in SceneView::buildSelectionRay, because that
// step genuinely needs the live GL matrices and the FLTK widget geometry.)

#ifndef VIEW_PICKING_H
#define VIEW_PICKING_H

#include "scene/V3.h"
#include "scene/M33.h"

namespace sg {
namespace picking {

/**
 * Ray vs. oriented box, using the "slab method".
 * https://en.wikipedia.org/wiki/Slab_method
 *
 * The box is an axis-aligned cube of the given `size`, rotated by `rotation`
 * and centered at `center` (this matches how a terminal is drawn). We bring the
 * ray into the box's local frame and intersect it there.
 *
 * params: origin,dir  the ray (dir should be unit length so outT is a distance)
 *         center      box center, world space
 *         size        box dimensions (full width/height/depth)
 *         rotation    box orientation, world space
 *         outT        (out) distance along the ray to the entry point
 * returns true if the ray enters the box; use outT to find the closest hit.
 */
bool rayHitBox(const V3& origin, const V3& dir,
               const V3& center, const V3& size, const M33& rotation,
               float& outT);

/**
 * Ray vs. the flat ground plane (y = 0).
 *
 * params: origin,dir  the ray
 *         outPoint    (out) where the ray meets the ground, world space
 * returns true if the ray actually meets the ground; false if it runs parallel
 *         to the ground or the intersection is behind the ray's origin.
 */
bool rayHitGround(const V3& origin, const V3& dir, V3& outPoint);

} // namespace picking
} // namespace sg

#endif // VIEW_PICKING_H
