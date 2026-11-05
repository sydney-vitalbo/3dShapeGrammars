// SceneView.cpp

#include "view/SceneView.h"

#include <FL/Fl.H>
#include <FL/gl.h>
#include <FL/Fl_PNG_Image.H> // FLTK's bundled PNG decoder (no extra dependency)

#include <cstdlib>   // std::abs
#include <vector>    // repack pixel rows for glTexImage2D
#include <cmath>     // std::abs(float)
#include <limits>    // std::numeric_limits
#include <utility>   // std::swap
#include <algorithm> // std::min, std::max
#include <stdexcept>

//macOS compatibility
#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#endif

namespace sg {

namespace {
AABB terminalBounds(const std::vector<ShapeNode>& terminals, int buildingId = -1) {
    AABB bounds;
    for (const ShapeNode& terminal : terminals) {
        if (buildingId >= 0 && terminal.buildingId != buildingId) continue;
        V3 center = terminal.position, half = terminal.size;
        half = half * 0.5f;
        M33 rotation = terminal.rotation;
        for (int x : {-1, 1}) for (int y : {-1, 1}) for (int z : {-1, 1})
            bounds.AddPoint(center + rotation * V3(x*half[0], y*half[1], z*half[2]));
    }
    return bounds;
}

void requireInsideGrid(AABB bounds, float limit) {
    if (bounds.minC[0] < -limit-1e-4f || bounds.maxC[0] > limit+1e-4f ||
        bounds.minC[2] < -limit-1e-4f || bounds.maxC[2] > limit+1e-4f)
        throw std::invalid_argument("This size extends beyond the grid. Move the building inward or reduce its width/depth.");
}
} // namespace

SceneView::SceneView(int x, int y, int w, int h, const char* label)
    : Fl_Gl_Window(x, y, w, h, label) { //pass position/size up to the FLTK base class
    //tell FLTK what features we want in openGL surface:
    //  - color (red/green/blue) rendering
    //  - a "depth buffer" so OpenGL knows which surfaces are in front
    //  - double buffering... draw the next frame off-screen, then swap it in to avoid flicker
    mode(FL_RGB | FL_DEPTH | FL_DOUBLE);

    //build the two primitives ONCE here, and later rescale/position them each frame
    boxMesh = primitives::makeUnitBox();
    cylinderMesh = primitives::makeUnitCylinder(32); //32=radial segments in the cylinder (smoothness)

    // The UI adds the initial preset through addBuilding().
}

std::size_t SceneView::addBuilding(bool apartment, const building_rules::Parameters& parameters) {
    ShapeNode root = apartment ? building_rules::makeApartment(parameters)
                               : building_rules::makeOffice(parameters);
    engine.GenerateTree(&root);
    const auto pieces = engine.TerminalsForRoot(root);
    AABB bounds = terminalBounds(pieces);
    requireInsideGrid(bounds, grid.halfExtent);

    // Find a free grid location, trying the origin and then nearby X positions
    // before starting another row. Existing buildings keep their placement.
    std::vector<AABB> occupied;
    for (const ShapeNode& existing : engine.roots)
        occupied.push_back(terminalBounds(engine.TerminalsForRoot(existing)));
    std::vector<float> coordinates{0};
    const int cells = static_cast<int>(std::floor(grid.halfExtent / grid.spacing));
    for (int i = 1; i <= cells; ++i) {
        coordinates.push_back(i * grid.spacing);
        coordinates.push_back(-i * grid.spacing);
    }
    bool found = false;
    V3 placement(0, 0, 0);
    for (float z : coordinates) {
        for (float x : coordinates) {
            if (bounds.minC[0]+x < -grid.halfExtent || bounds.maxC[0]+x > grid.halfExtent ||
                bounds.minC[2]+z < -grid.halfExtent || bounds.maxC[2]+z > grid.halfExtent) continue;
            bool overlaps = false;
            for (AABB other : occupied) {
                if (bounds.maxC[0]+x > other.minC[0]-grid.spacing &&
                    bounds.minC[0]+x < other.maxC[0]+grid.spacing &&
                    bounds.maxC[2]+z > other.minC[2]-grid.spacing &&
                    bounds.minC[2]+z < other.maxC[2]+grid.spacing) {
                    overlaps = true;
                    break;
                }
            }
            if (overlaps) continue;
            placement = V3(x, 0, z);
            found = true;
            break;
        }
        if (found) break;
    }
    if (!found) throw std::invalid_argument("The grid has no free space for another building.");
    root.position = root.position + placement;
    const int buildingId = engine.AddRoot(std::move(root));
    selectBuilding(buildingId);
    redraw();
    return pieces.size();
}

std::size_t SceneView::setBuilding(bool apartment, const building_rules::Parameters& parameters) {
    const ShapeNode* current = engine.FindRoot(selectedRootId);
    if (!current) throw std::invalid_argument("Select a building before applying changes.");
    // Build a candidate with the same placement/orientation. Commit only after
    // validation, preserving every other root and its generated geometry.
    ShapeNode root = apartment ? building_rules::makeApartment(parameters)
                               : building_rules::makeOffice(parameters);
    V3 previousPosition = current->position, previousSize = current->size;
    root.position = V3(previousPosition[0], previousPosition[1]-previousSize[1]/2+root.size[1]/2,
                       previousPosition[2]);
    root.rotation = current->rotation;
    engine.GenerateTree(&root);
    const auto pieces = engine.TerminalsForRoot(root);
    requireInsideGrid(terminalBounds(pieces), grid.halfExtent);
    engine.ReplaceRoot(selectedRootId, std::move(root));
    movingBuilding = false;
    if (selectionChanged) selectionChanged();
    redraw();
    return pieces.size();
}

bool SceneView::getSelectedBuilding(bool& apartment, building_rules::Parameters& parameters) const {
    const ShapeNode* root = engine.FindRoot(selectedRootId);
    if (!root) return false;
    V3 size = root->size;
    apartment = root->symbol == Symbol::Apartment;
    parameters.floors = root->building.floors;
    parameters.bays = root->building.bays;
    parameters.width = size[0];
    parameters.depth = size[2];
    parameters.floorHeight = (size[1]-building_rules::roofHeight) / parameters.floors;
    parameters.balconies = root->building.balconies;
    return true;
}

void SceneView::selectBuilding(int buildingId) {
    if (buildingId != -1 && !engine.FindRoot(buildingId))
        throw std::invalid_argument("Building no longer exists.");
    if (selectedRootId == buildingId) return;
    selectedRootId = buildingId;
    movingBuilding = false;
    if (selectionChanged) selectionChanged();
    redraw();
}

bool SceneView::deleteSelectedBuilding() {
    if (selectedRootId < 0) return false;
    const bool removed = engine.RemoveRoot(selectedRootId);
    selectBuilding(-1); // Clear highlighting/drag state and refresh the sidebar.
    return removed;
}

void SceneView::onSelectionChanged(std::function<void()> callback) {
    selectionChanged = std::move(callback);
}

void SceneView::initGLState() { //enabling opengl features

    //Depth testing: for each pixel, keep whatever is nearest the camera and discard anything behind it
    glEnable(GL_DEPTH_TEST);

    //Lighting
    glEnable(GL_LIGHTING);
    glEnable(GL_LIGHT0);
    GLfloat lightPos[]  = {6.0f, 10.0f, 8.0f, 1.0f}; // where the light sits (up/right/front)
    GLfloat lightDiff[] = {0.9f, 0.9f, 0.9f, 1.0f};  // "diffuse" = the main directional shading
    GLfloat lightAmb[]  = {0.25f, 0.25f, 0.25f, 1.0f};// "ambient" = fill light so shadows aren't pure black
    glLightfv(GL_LIGHT0, GL_POSITION, lightPos);
    glLightfv(GL_LIGHT0, GL_DIFFUSE, lightDiff);
    glLightfv(GL_LIGHT0, GL_AMBIENT, lightAmb);

    //tint each shape with its "material"
    glEnable(GL_COLOR_MATERIAL);
    glColorMaterial(GL_FRONT_AND_BACK, GL_AMBIENT_AND_DIFFUSE);

    //tells OpenGL to re-normalize the normal vectors (whenever a scale or something happens) so lighting stays correct
    glEnable(GL_NORMALIZE);

    //background color of the window
    glClearColor(0.12f, 0.13f, 0.16f, 1.0f); //dark gray-blue
}

// Decode one PNG with FLTK and upload it as an OpenGL texture for `material`
//https://www.fltk.org/newsgroups.php?s0+gfltk.general+v39519+T0
void SceneView::loadTexture(Material material, const char* path) {
    Fl_PNG_Image img(path);
    if (img.fail() || img.w() <= 0 || img.h() <= 0) {
        return; // file missing/unreadable -> leave this material untextured
    }

    const int w = img.w();
    const int h = img.h();
    const int d = img.d();  // channels: 1=gray,3=RGB,4=RGBA
    const int ld = img.ld() ? img.ld() : w * d; // row stride (may include padding)
    const unsigned char* src = reinterpret_cast<const unsigned char*>(img.data()[0]);

    // Repack into RGB/RGBA
    const int outCh = (d == 4) ? 4 : 3;
    std::vector<unsigned char> pixels(static_cast<size_t>(w) * h * outCh);
    for (int y = 0; y < h; ++y) {
        const unsigned char* row = src + static_cast<size_t>(y) * ld;
        unsigned char* out = pixels.data() + static_cast<size_t>(y) * w * outCh;
        for (int x = 0; x < w; ++x) {
            const unsigned char* p = row + static_cast<size_t>(x) * d;
            if (d == 1) { out[0] = out[1] = out[2] = p[0]; }       // gray -> RGB
            else { out[0]=p[0]; out[1]=p[1]; out[2]=p[2]; if (outCh==4) out[3]=p[3]; }
            out += outCh;
        }
    }

    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1); // our buffer is tightly packed
    const GLint fmt = (outCh == 4) ? GL_RGBA : GL_RGB;
    // gluBuild2DMipmaps handles non-power-of-two sizes and builds mipmaps, so the
    // texture stays clean when the building is small/far on screen.
    gluBuild2DMipmaps(GL_TEXTURE_2D, fmt, w, h, fmt, GL_UNSIGNED_BYTE, pixels.data());
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glBindTexture(GL_TEXTURE_2D, 0);

    materialTextures[static_cast<int>(material)] = tex;
}

// Load one texture per building material
void SceneView::loadTextures() {
    loadTexture(Material::Stone, "src/textures/Stone.png");
    loadTexture(Material::Glass, "src/textures/Glass.png");
    loadTexture(Material::Wood,  "src/textures/Wood.png");
    // Material::None (roof deck) stays untextured -> flat color.
}

// draw() is called by FLTK whenever the view needs repainting (first show,
// after a resize, or after we call redraw()). It paints exactly one frame.
void SceneView::draw() {
    //valid() = fltk helper
    //returns false the very first time and after the window is resized
    if (!valid()) {
        if (!glInitialized) {
            initGLState();
            glInitialized = true;
        }
        //textures need a current GL context, which exists here (inside draw)
        if (!texturesLoaded) {
            loadTextures();
            texturesLoaded = true;
        }
        //on resize tell OpenGL the new size, and recompute the projection so the picture's aspect ratio matches the window and nothing looks stretched
        // https://stackoverflow.com/questions/29153163/opengl-how-to-fit-a-window-on-resize
        glViewport(0, 0, pixel_w(), pixel_h());
        float aspect = pixel_h() > 0 ? static_cast<float>(pixel_w()) / pixel_h() : 1.0f;
        camera.applyProjection(aspect);
    }

    //wipe last frame: color and depth buffers
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    //position the camera for this frame (sets the view matrix)
    camera.applyView();

    //draw the scene contents
    drawGroundGrid();
    drawGrammarScene();
}

void SceneView::drawGroundGrid() {
    //rreference grid on the floor (y = 0)
    
    //lighting OFF so the lines are a flat, constant color
    glDisable(GL_LIGHTING);
    glColor3f(0.30f, 0.32f, 0.38f); // light gray
    glBegin(GL_LINES);
    //https://stackoverflow.com/questions/29455751/how-to-calculate-the-half-extents-of-a-cube
    for (float c = -grid.halfExtent; c <= grid.halfExtent + 1e-3f; c += grid.spacing) {
        // lines parallel to Z
        glVertex3f(c, 0.0f, -grid.halfExtent);
        glVertex3f(c, 0.0f,  grid.halfExtent);
        // lines parallel to X
        glVertex3f(-grid.halfExtent, 0.0f, c);
        glVertex3f( grid.halfExtent, 0.0f, c);
    }
    glEnd();
    glEnable(GL_LIGHTING);
}

void SceneView::drawGrammarScene() {
    //each terminal has a world-space position, a size, and a material. draw the unit box moved+scaled to match, tinted by material
    for (ShapeNode& t : engine.terminals) {
        const bool buildingSelected = t.buildingId == selectedRootId;
        //map the grammar Material tag to an RGB tint for this terminal
        float r = 0.8f, g = 0.8f, b = 0.8f;
        switch (t.material) {
            case Material::Stone: r = 0.75f; g = 0.79f; b = 0.82f; break; // light gray stone
            case Material::Glass: r = 0.20f; g = 0.56f; b = 0.68f; break; // blue-teal glass
            case Material::Wood:  r = 0.18f; g = 0.25f; b = 0.30f; break; // dark trim
            case Material::None:  r = 0.19f; g = 0.22f; b = 0.24f; break; // roof deck
        }

        //does this material have a texture loaded?
        const unsigned int tex = materialTextures[static_cast<int>(t.material)];

        if (tex != 0) {
            //Textured: bind it and let OpenGL AUTO-GENERATE texture coordinates
            //from the vertex positions (object-linear texgen)
            if (buildingSelected) { r = std::min(1.0f, r*0.6f+0.45f);
                                    g = std::min(1.0f, g*0.6f+0.38f);
                                    b = std::min(1.0f, b*0.6f+0.10f); glColor3f(r,g,b); }
            else                  { glColor3f(1.0f, 1.0f, 1.0f); }

            glEnable(GL_TEXTURE_2D);
            glBindTexture(GL_TEXTURE_2D, tex);
            //s follows object X, t follows object Y. Folding the terminal's size
            //into the planes makes the texture tile ~once per world unit, so the
            //scale looks consistent across big walls and tiny trim pieces
            const GLfloat sPlane[4] = {t.size[0], 0.0f, 0.0f, 0.0f};
            const GLfloat tPlane[4] = {0.0f, t.size[1], 0.0f, 0.0f};
            glTexGeni(GL_S, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
            glTexGeni(GL_T, GL_TEXTURE_GEN_MODE, GL_OBJECT_LINEAR);
            glTexGenfv(GL_S, GL_OBJECT_PLANE, sPlane);
            glTexGenfv(GL_T, GL_OBJECT_PLANE, tPlane);
            glEnable(GL_TEXTURE_GEN_S);
            glEnable(GL_TEXTURE_GEN_T);
        } else {
            //Untextured: keep the existing flat-color look, brightened when selected.
            if (buildingSelected) {
                r = std::min(1.0f, r * 0.6f + 0.45f);
                g = std::min(1.0f, g * 0.6f + 0.38f);
                b = std::min(1.0f, b * 0.6f + 0.10f);
            }
            glColor3f(r, g, b);
        }

        //meshes are unit-sized and centered at the origin
        //to place a copy somewhere, temporarily change openGL's "modelview" transform, draw, then undo the change:
        //   glPushMatrix() : save the current transform
        //   glTranslatef() : move the origin for whatever we draw next
        //   glScalef()     : resize whatever we draw next
        //   <draw>
        //   glPopMatrix()  : restore the saved transform
        //https://www.scribd.com/document/135598965/NeHe-OpenGL-Tutorial#page=9

        glPushMatrix();
        glTranslatef(t.position[0], t.position[1], t.position[2]); //world position
        // M33 stores rows; OpenGL consumes a column-major 4x4 matrix.
        const float rotation[16] = {
            t.rotation[0][0], t.rotation[1][0], t.rotation[2][0], 0,
            t.rotation[0][1], t.rotation[1][1], t.rotation[2][1], 0,
            t.rotation[0][2], t.rotation[1][2], t.rotation[2][2], 0,
            0, 0, 0, 1
        };
        glMultMatrixf(rotation);
        glScalef(t.size[0], t.size[1], t.size[2]);                 //box dimensions
        boxMesh.draw(); // all current terminals are box shapes
        glPopMatrix();

        //turn texturing back off so the next piece (or the grid/highlight) isn't accidentally textured
        if (tex != 0) {
            glDisable(GL_TEXTURE_GEN_S);
            glDisable(GL_TEXTURE_GEN_T);
            glDisable(GL_TEXTURE_2D);
        }
    }

    //when selected, add a single outline around the whole building
    if (selectedRootId >= 0) {
        drawSelectionHighlight();
    }
}

//draw highlight box around the whole selected building
void SceneView::drawSelectionHighlight() {
    const ShapeNode* root = engine.FindRoot(selectedRootId);
    if (!root) return;
    // Full geometry keeps the selected outline stable even when a join hides
    // a whole side (or another building completely contains this root).
    AABB bounds = terminalBounds(engine.TerminalsForRoot(*root));

    //2) pad the box slightly so the outline floats just outside
    const float pad = 0.03f;
    V3 lo = bounds.minC - V3(pad, pad, pad);
    V3 hi = bounds.maxC + V3(pad, pad, pad);

    //3) the 8 corners of that bounding box
    const V3 corners[8] = {
        V3(lo[0], lo[1], lo[2]), V3(hi[0], lo[1], lo[2]),
        V3(hi[0], hi[1], lo[2]), V3(lo[0], hi[1], lo[2]),
        V3(lo[0], lo[1], hi[2]), V3(hi[0], lo[1], hi[2]),
        V3(hi[0], hi[1], hi[2]), V3(lo[0], hi[1], hi[2])
    };
    //the 12 edges, given as pairs of corner indices
    const int edges[12][2] = {
        {0,1},{1,2},{2,3},{3,0}, // bottom face
        {4,5},{5,6},{6,7},{7,4}, // top face
        {0,4},{1,5},{2,6},{3,7}  // vertical pillars
    };

    //4) color the edges of BB
    glDisable(GL_LIGHTING);        // outline is a flat, constant color
    glDisable(GL_DEPTH_TEST);      // draw on top so the border is never hidden
    glColor3f(1.0f, 0.85f, 0.25f); // highlight
    glLineWidth(2.0f);             // a bit thicker
    glBegin(GL_LINES);
    for (const auto& e : edges) {
        V3 a = corners[e[0]];
        V3 b = corners[e[1]];
        glVertex3f(a[0], a[1], a[2]);
        glVertex3f(b[0], b[1], b[2]);
    }
    glEnd();
    glLineWidth(1.0f);             // restore default line width
    glEnable(GL_DEPTH_TEST);       // restore normal depth testing
    glEnable(GL_LIGHTING);         // restore lighting for the rest of the scene
}

// Turn a 2D mouse click into a 3D "selection ray" shooting into the scene.
// gluUnProject: give it a window pixel plus a depth, and it tells you the matching 3D world point
// Outputs: outOrigin = where the ray starts, outDir = unit direction it travels.
// https://www.khronos.org/opengl/wiki/GluProject_and_gluUnProject_code
void SceneView::buildSelectionRay(int mouseX, int mouseY, V3& outOrigin, V3& outDir) {
    // gluUnProject needs the same things OpenGL used to draw this frame:
    //   viewport   = the pixel rectangle we render into (x, y, width, height)
    //   modelview  = where the camera is / how the world is positioned
    //   projection = the perspective transform
    GLint viewport[4];
    GLdouble modelview[16];
    GLdouble projection[16];
    glGetIntegerv(GL_VIEWPORT, viewport);
    glGetDoublev(GL_MODELVIEW_MATRIX, modelview);
    glGetDoublev(GL_PROJECTION_MATRIX, projection);

    //fix coord-system before we can use the mouse pixel:
    //  (1) Y is flipped. FLTK measures the mouse from the TOP-LEFT (y grows
    //      downward), but OpenGL measures its window from the BOTTOM-LEFT (y
    //      grows upward). So flip it: glY = height - mouseY
    //
    //  (b) Retina scaling. The framebuffer has more pixels than the of mouse space's size
    //      So have to scale the mouse coordinates up by that ratio
    const float sx = pixel_w() > 0 && w() > 0 ? static_cast<float>(pixel_w()) / w() : 1.0f;
    const float sy = pixel_h() > 0 && h() > 0 ? static_cast<float>(pixel_h()) / h() : 1.0f;
    const double winX = viewport[0] + (mouseX - x()) * sx;
    const double winY = viewport[1] + viewport[3] - (mouseY - y()) * sy;

    // Unproject the same pixel at two depths to get two points:
    //   depth 0.0 -> point on the near plane (close to the camera)
    //   depth 1.0 -> point on the far plane
    GLdouble nx, ny, nz, fx, fy, fz;
    gluUnProject(winX, winY, 0.0, modelview, projection, viewport, &nx, &ny, &nz);
    gluUnProject(winX, winY, 1.0, modelview, projection, viewport, &fx, &fy, &fz);

    //ray starts at the near point and ends at far point:
    outOrigin = V3(static_cast<float>(nx), static_cast<float>(ny), static_cast<float>(nz));
    V3 farPoint(static_cast<float>(fx), static_cast<float>(fy), static_cast<float>(fz));
    outDir = (farPoint - outOrigin).Normalize();
}

//Determine if selection ray hit this terminal
//"slab method": https://en.wikipedia.org/wiki/Slab_method
// Returns true on a hit and writes the entry distance to outT (used to pick the closest terminal)
bool SceneView::rayHitTerminal(const V3& origin, const V3& dir,
                                const ShapeNode& terminal, float& outT) const {
    //V3/M33 math API has no const methods, so copy into mutable locals
    V3 org = origin, direction = dir;
    V3 center = terminal.position;
    V3 size = terminal.size;
    M33 rot = terminal.rotation;
    M33 inv = rot.Transpose(); // world-orientation -> local-orientation (inverse)

    // Move the ray into the box's local space: subtract the box center (so the
    // box sits at the origin) and un-rotate it. Now the box is axis-aligned and
    // spans [-half, +half] on each axis
    V3 o = inv * (org - center); // ray start,  in local space
    V3 d = inv * direction;      // ray heading, in local space
    V3 half = size * 0.5f;       // half the box size on each axis

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

//Determine what the user clicked
bool SceneView::pickComponent(int mouseX, int mouseY) {
    //this runs from the mouse-event handler (not draw()) so the GL context
    // may not be active. make_current() activates it
    make_current();
    float aspect = pixel_h() > 0 ? static_cast<float>(pixel_w()) / pixel_h() : 1.0f;
    camera.applyProjection(aspect); // projection must be current for gluUnProject
    camera.applyView();             // modelview = this frame's view

    //build the ray that goes from the camera through the clicked pixel
    V3 origin, dir;
    buildSelectionRay(mouseX, mouseY, origin, dir);

    // Walk every terminal and remember the nearest one the ray enters
    // NOTE: supports boxes currently- cylinders fall back to their bounding box, which is fine for whole-building selection?
    float bestT = std::numeric_limits<float>::max(); // best (smallest) distance so far
    int hitBuildingId = -1;
    for (const ShapeNode& t : engine.terminals) {
        float tHit;
        // rayHitTerminal returns true + the entry distance if the ray pierces
        // this box; we keep it only if it's closer than anything found before
        if (rayHitTerminal(origin, dir, t, tHit) && tHit < bestT) {
            bestT = tHit;
            hitBuildingId = t.buildingId;
        }
    }

    selectBuilding(hitBuildingId);
    return hitBuildingId >= 0;
}

// Find where a ray crosses the flat ground plane (y = 0)
//
// just need the t where the ray's y-component becomes 0:
//     origin.y + t * dir.y = 0   ->   t = -origin.y / dir.y
// If dir.y is ~0 the ray runs parallel to the ground, if t is negative the ground is behind the camera (no hit)
bool SceneView::rayHitGround(const V3& origin, const V3& dir, V3& outPoint) const {
    V3 o = origin, d = dir; // local mutable copies (V3 API is non-const)
    const float dy = d[1];
    if (std::abs(dy) < 1e-8f) return false;     // parallel to the ground
    const float t = -o[1] / dy;
    if (t < 0.0f) return false;                 // ground is behind the camera
    outPoint = o + d * t;                       // the point where y == 0
    return true;
}

// Snap a ground point to the nearest grid intersection, then confine it so the
// WHOLE building stays inside the grid.
V3 SceneView::snapToGrid(const V3& point) const {
    V3 p = point; // local mutable copy
    const ShapeNode* root = engine.FindRoot(selectedRootId);
    if (!root) return p;
    const float step = grid.spacing;            // one grid cell, the snap unit
    float sx = std::round(p[0] / step) * step;  // round to nearest grid step
    float sz = std::round(p[2] / step) * step;

    // keep the placement inside the grid's extent so you can't drag off the edge
    AABB bounds = terminalBounds(engine.TerminalsForRoot(*root));
    V3 rootPosition = root->position;
    // Use asymmetric bounds (balconies/canopies project forward) and round the
    // allowed interval inward so clamping also preserves grid alignment.
    const float minX = std::ceil((-grid.halfExtent-bounds.minC[0]+rootPosition[0])/step)*step;
    const float maxX = std::floor((grid.halfExtent-bounds.maxC[0]+rootPosition[0])/step)*step;
    const float minZ = std::ceil((-grid.halfExtent-bounds.minC[2]+rootPosition[2])/step)*step;
    const float maxZ = std::floor((grid.halfExtent-bounds.maxC[2]+rootPosition[2])/step)*step;
    sx = std::max(minX, std::min(maxX, sx));
    sz = std::max(minZ, std::min(maxZ, sz));

    return V3(sx, p[1], sz);
}

// fltk's input callback
// return 1 if we consumed the input
//https://www.fltk.org/doc-1.3/events.html
int SceneView::handle(int event) {
    switch (event) {
    case FL_PUSH: //mouse button was pressed
        //remember where the drag started
        lastMouseX = Fl::event_x(); //current cursor pos in pixels
        lastMouseY = Fl::event_y();
        if (Fl::event_button() == FL_LEFT_MOUSE) {
            //left click does the selection test at the clicked pixel.
            // (pickComponent leaves the GL context current and the camera matrices applied, so we can build another ray right after it)
            bool hitBuilding = pickComponent(Fl::event_x(), Fl::event_y());

            if (hitBuilding) {
                //clicked ON the building -> begin MOVING it (not camera orbiting)
                V3 origin, dir, groundPt;
                buildSelectionRay(Fl::event_x(), Fl::event_y(), origin, dir);
                if (rayHitGround(origin, dir, groundPt)) {
                    V3 position = engine.FindRoot(selectedRootId)->position;
                    grabOffset = position - groundPt;
                    movingBuilding = true;
                }
                dragging = false; // don't orbit while moving the building
            } else {
                //clicked empty space ->  orbit the camera once the mouse moves (handled below in FL_DRAG).
                movingBuilding = false;
                dragging = true;
            }
            //the selection highlight changed, so repaint
            redraw();
        } else if (Fl::event_button() == FL_MIDDLE_MOUSE ||
                   Fl::event_button() == FL_RIGHT_MOUSE) {
            panning = true;
        }
        return 1; // consume so we keep getting FL_DRAG/FL_RELEASE

    case FL_DRAG: {
        //how far the mouse moved since the last event (delta x / delta y)
        int dx = Fl::event_x() - lastMouseX;
        int dy = Fl::event_y() - lastMouseY;
        lastMouseX = Fl::event_x();
        lastMouseY = Fl::event_y();

        if (movingBuilding) {
            //dragging the selected building along the ground. Rebuild the ray for the cursor's new position and see where it meets the ground
            make_current();
            float aspect = pixel_h() > 0 ? static_cast<float>(pixel_w()) / pixel_h() : 1.0f;
            camera.applyProjection(aspect); // matrices must be current for the ray
            camera.applyView();

            V3 origin, dir, groundPt;
            buildSelectionRay(Fl::event_x(), Fl::event_y(), origin, dir);
            if (rayHitGround(origin, dir, groundPt)) {
                //New placement = ground point + the grab offset we stored, so the building keeps the same spot under the cursor
                //then snap to the grid
                V3 placement = groundPt + grabOffset;
                engine.MoveRoot(selectedRootId, snapToGrid(placement));
                redraw();
            }
        } else if (dragging) {
            //pixels -> degrees
            camera.orbit(dx * 0.4f, -dy * 0.4f);
            redraw();
        } else if (panning) {
            camera.pan(static_cast<float>(dx), static_cast<float>(dy));
            redraw();
        }
        return 1;
    }

    case FL_RELEASE:
        dragging = false;
        panning = false;
        movingBuilding = false; // stop moving the building when the button is let go
        return 1;

    case FL_MOUSEWHEEL: {
        // Fl::event_dy() is +1 when scrolling down (zoom out), -1 up (zoom in)
        int dy = Fl::event_dy();
        float factor = (dy > 0) ? 1.1f : 0.9f;
        for (int i = 1; i < std::abs(dy); ++i) factor *= (dy > 0) ? 1.1f : 0.9f;
        camera.zoom(factor);
        redraw();
        return 1;
    }

    case FL_FOCUS:
    case FL_UNFOCUS:
        return 1; // accept focus so keyboard/wheel events reach us

    default:
        return Fl_Gl_Window::handle(event);
    }
}

} // namespace sg
