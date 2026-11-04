# Automatic building joining

Each scene root owns an optional `rootJoin` record with its structural world
AABB and `overlapMask`. Descendants have no populated spatial record. Bounds
come from the root's dimensions and pose, excluding projecting decorations, and
stay stable when components are disabled.

`AddRoot`, `ReplaceRoot`, and snapped `MoveRoot` calls rebuild the render cache.
`RebuildTerminals()` first checks all root pairs. Faces with positive-area
contact or intersection contribute to both roots' masks. Edge/corner-only
contact does not join. A completely contained root is disabled; coincident roots
keep the older root visible.

Mask bits represent world directions: Right +X, Left -X, Top +Y, Bottom -Y,
Front +Z, Back -Z. Rules tag exterior components with building-local
`exteriorFace` values; the engine maps these through the root's rotation.
Changed masks, regenerated trees, or changed orientations trigger propagation
of `disabled` through the affected trees. Disabled parents disable their entire
subtree. Separation recomputes masks from scratch and restores components.

The body and floor band use separate shell pieces so one side can be disabled.
Wall-region tags also suppress that side's facade details. Roof parapets have
side tags, and the roof subtree has a Top tag. Visible terminal collection omits
disabled nodes, so rendering and picking both use the updated geometry.
`TerminalsForRoot()` includes disabled geometry for consistent grid limits,
automatic placement, and selection outlines.

This is deliberately coarse whole-face joining. Partial contact, including a
short building touching a taller one, suppresses the corresponding entire side.
Rotated-root AABBs are conservative. Faces are not clipped to overlap patches,
and intersecting solids are not converted into a Boolean mesh union.
