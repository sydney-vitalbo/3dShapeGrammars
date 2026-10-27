# Office and apartment rules — handoff draft

This contribution contains the authored building recipes and the operation
interface they require. It does not implement an engine, renderer, GUI, or joining.
The temporary preview and operation implementations have been removed.

## Files and ownership

| File | Purpose |
| --- | --- |
| `src/grammar/BuildingRules.h` | Building parameters and the two public rule entry points. |
| `src/grammar/BuildingRules.cpp` | Borna's building recipes, with productions commented above their implementation. |
| `src/grammar/Operations.h` | Agreed declarations for the four operations Mridu's engine supplies. |

Mridu owns operation implementations and tree expansion/traversal. Sydney owns
rendering and interaction. The existing team source files and build configuration
remain unchanged. `Rules.cpp` still contains the original facade example.

## Rule definitions

```text
Office -> GroundFloor + repeated OfficeFloor + FlatRoof
Apartment -> GroundFloor + repeated ApartmentFloor + FlatRoof
Floor -> WallRegion + fixed-height FloorBand
WallRegion -> solid wall + front facade + rear facade
GroundFacade -> left windows + Entrance + right windows
OfficeFacade -> repeated wide glass bays
ApartmentFacade -> repeated apartment bays
ApartmentBay -> smaller window | balcony door + Balcony
Window -> frame + glass + center mullion + sill
Entrance -> framed glass door + shallow canopy
Balcony -> slab + low front wall + two side walls
FlatRoof -> deck + four thin parapet walls
```

`makeOffice(parameters)` and `makeApartment(parameters)` construct local
`ShapeNode` trees. The names in the production comments describe the recipes;
they do not require new entries in the existing `Symbol` enum.

Both buildings have front and rear windows, plain side walls, and a rectangular
footprint. Balcony selection is explicit and deterministic. Balconies occur only
on upper front apartment floors; the ground floor always uses its entrance rule.
Office balconies are ignored. All geometry uses boxes and existing material tags.

## Parameters and proportions

| Parameter | Default | Accepted range / meaning |
| --- | --- | --- |
| `floors` | 3 | 1–12, including the ground floor. |
| `bays` | 3 | 2–8, repeated sections on each upper facade. |
| `width` | 6 | 4–12 world units. |
| `floorHeight` | 1.5 | 1.2–3 world units, shared by both families when placed together. |
| `depth` | 3 | 2–8 world units. |
| `balconies` | true | Selects the apartment's balcony alternative. |

Floating-point dimensions must be finite. Invalid parameters throw
`std::invalid_argument` before construction. The front ground floor has
`max(1, bays / 2)` windows on each side of the entrance; it need not match the
upper-floor bay count.

Each floor includes a 0.12-unit band; the roof adds 0.24 units above the floors.
Total height is `floors * floorHeight + 0.24`. The root center is at half that
height, keeping the base at ground level. Changing floor count repeats floors
instead of stretching them. Balcony projection is 0.65 units, with a 0.08-unit
slab and 0.43-unit guards. Roof edges stay inside the footprint.

## Operation contract for Mridu

Use the global `ShapeNode` in `src/grammar/ShapeNode.h`. The root uses world units;
descendants use fractions of their parent's dimensions. Local scopes are centered
at zero and span `[-0.5, +0.5]` on each axis. Resolve a child per coordinate as:

```text
childWorldPosition = parentWorldPosition + childLocalPosition * parentWorldSize
childWorldSize = parentWorldSize * childLocalSize
```

| Operation | Required behavior |
| --- | --- |
| `subdivide(axis, extent, splits)` | Allocate fixed lengths, then share remaining space by relative weights. Return normalized child scopes filling the parent along the chosen axis. |
| `repeat(axis, count, gap)` | Return equal normalized child scopes filling the chosen axis, with `count - 1` gaps and no outer margins. These rules use the default zero gap. |
| `compose(parent, children)` | Return a nonterminal parent containing those children, preserving its local transform, symbol, and material. |
| `terminal(material, size, position)` | Return a terminal `Symbol::Slab` with those local dimensions/center, the supplied material, and no children. |

For example, subdividing extent 10 with fixed 2, relative 1, relative 3 produces
physical widths 2, 2, 6 and normalized widths 0.2, 0.2, 0.6. All-fixed splits
must fill the supplied extent; allow float roundoff in that sum. Split lengths
and weights must be positive; repetition needs a positive count and gaps that
leave positive space for each child. Invalid inputs should throw
`std::invalid_argument`.

Subdivision/repetition children retain the existing default symbol/material,
have no children, and start nonterminal. The rule functions subsequently expand
or mark them terminal. Positions on other axes remain zero and sizes remain one.
These operations do not modify caller-owned inputs. Tree IDs are assigned by the
engine, not by these building rules.

## Integration status

The new rules are deliberately **not linked into the current application yet**.
The original application continues to build and run its existing facade. To use
the new rules, Mridu supplies the operation definitions and the team adds those
definitions plus `BuildingRules.cpp` to the application build.

The returned tree already contains its authored children. The current
`GrammarEngine::Expand` calls the old `Rules::Expand` and does not traverse these
stored children, so it must not be used unchanged on this tree. Engine integration
needs to preserve the hierarchy, assign IDs, and collect world-space terminal
leaves by walking `children`. Replacing a generated building must discard its old
leaves. The engine may implement the four interface functions as adapters to its
own classes; this contract does not prescribe its internal organization.

For adjacent placement, move the root centers according to their footprint widths
and keep the same front alignment, floor height, and ground level. Placement is
separate from these recipes. Wall removal, mesh joining, picking, and dragging
remain outside this contribution.

## Appearance reference for rendering

The approved preview used these RGB colors. Color assignment belongs to the
renderer; the rules store only the existing material tags. Retain the selected
building family alongside its tree if reproducing the two palettes.

| Material tag | Office RGB | Apartment RGB |
| --- | --- | --- |
| Stone | (0.75, 0.79, 0.82) | (0.84, 0.78, 0.67) |
| Glass | (0.20, 0.56, 0.68) | (0.30, 0.51, 0.60) |
| Wood | (0.18, 0.25, 0.30) | (0.48, 0.33, 0.24) |
| None (roof deck) | (0.19, 0.22, 0.24) | (0.19, 0.22, 0.24) |

## Review and validation

Before cleanup, both recipes were built, rendered, and reviewed in the temporary
preview. Checks covered floor growth, the one-floor case, balcony selection,
independent parameters, placement with touching footprints, and regeneration.
Those preview/helper tests are not part of this handoff.

The final rule source can be checked without an engine implementation:

```sh
c++ -std=c++17 -fsyntax-only -I src -I src/scene src/grammar/BuildingRules.cpp
```

This checks compilation, not linkage or rendering. Integration testing resumes
when Mridu supplies the operations and connects tree traversal to the renderer.
