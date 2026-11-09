# Automatic building joining

Each individual building root owns a `rootJoin` record with its structural
world AABB and `overlapMask`, including when it is a child of a joined unit.
Grammar descendants have no populated spatial record. Bounds exclude projecting
decorations and stay stable when components are disabled. A JoinedUnit root
also caches the union of its members' structural AABBs for the selection outline.

`AddRoot`, `ReplaceRoot`, and snapped `MoveRoot` calls rebuild the render cache.
`RebuildTerminals()` first checks all root pairs. Faces with positive-area
contact or intersection contribute to both roots' masks. Edge/corner-only
contact does not join. A completely contained root is disabled; coincident roots
keep the older root visible.

Buildings connected by vertical shared faces become a single `JoinedUnit` in
`engine.roots`. Its children are the original building roots, with their original
IDs, absolute sizes, and positions relative to the unit. The unit gets its own
unique ID; third and later members become additional direct children. Unit IDs
survive movement, parameter edits, and membership changes while at least two
members remain connected. Disconnected components become separate scene roots.
The engine temporarily flattens member poses to world space for the existing
join calculations, then groups the connected components without regenerating
the grammar. Render/picking copies use the unit ID, so selecting or dragging
any visible member selects or moves the entire unit.

Shared settings are per member: Apply regenerates each member with common
floors, upper bays, floor height, and any unlocked dimensions, preserving office
versus apartment type, rotation, horizontal placement, and base elevation.
Side-to-side joins lock each member's width; front-to-back joins lock depth.
Units containing both join directions lock both dimensions. Locked dimensions
retain each member's existing value, even when members have different sizes.
Total member height is `floors * floorHeight + roofHeight`.

To detach a member, select that building or a descendant in the derivation tree
and click **Detach member**. `DetachBuilding(buildingId)` searches the unit's
children by original ID and moves the building back into `roots` at the same
world pose. Regrouping with its former unit members is suppressed until each
pair separates, allowing it to be dragged away without immediately rejoining.
The detached member can still join a different building immediately. A later
contact with a former member can join it again. Deleting a selected unit deletes
all its members; `RemoveRoot(memberId)` can also delete an individual member.

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

Front/back slabs are an exception: while their face is shared, the engine
temporarily splits each slab vertically into a left beam, center wall, and
right beam. Only the center is disabled. Both full-height beams have a
0.1-by-0.1 cross-section and retain the slab's material and outside plane.
This applies to body walls, floor bands, and front/back roof parapets.
Separation restores the original single slab. Containment or a disabled parent
still hides the entire component, including its beams. Width and depth continue
to describe the original outside footprint.

Root records also store covered horizontal and vertical intervals for each
vertical face, including neighbours of unequal height. Axis-aligned walls
(including quarter-turn rotations) are split vertically and horizontally at those boundaries:
covered terminals are disabled and exposed terminals remain visible. Front/back
edge beams remain visible as described above. The same logic applies to side
walls, floor bands, and parapets. Sliding recomputes these pieces even when the
face mask stays unchanged; separation restores the original slab. Multiple
overlap rectangles may produce additional pieces. A single overlap crossing
one width boundary and one height boundary produces four wall terminals
(plus the two retained beams on front/back slabs). Slabs wholly above or below
the neighbour remain intact.

At a roof-to-floor wall join, roof parapet pieces take priority: the roof piece
stays visible and the overlapping floor-wall portion is disabled. Roof pieces
are removed only where the neighbour also has a roof. Each root overlap record
includes the neighbour's roof-start elevation, and traversal carries roof
ancestry, avoiding comparisons between individual terminal pairs. Existing
front/back edge-beam retention still applies to roof-to-roof removal.

WallRegion and facade containers stay enabled. Each Bay and Entrance is disabled
only when its world AABB (scope plus generated details) has positive overlap
with a removed wall rectangle along the wall and vertically. Depth is projected
away because facade details sit outside the wall thickness. Any partial overlap
hides the entire bay/entrance subtree; boundary-only contact leaves it visible.
Separation restores these components. Oblique rotations
retain the previous whole-face behaviour. Rotated-root
AABBs are conservative; intersecting solids are not converted into a Boolean
mesh union.
