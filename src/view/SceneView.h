// SceneView.h: the actual 3D viewport the user sees and interacts with

#ifndef VIEW_SCENEVIEW_H
#define VIEW_SCENEVIEW_H

#include <FL/Fl_Gl_Window.H>

#include "scene/Camera.h"
#include "scene/Mesh.h"
#include "scene/Primitives.h"
#include "scene/SceneController.h" //the scene model this view renders + edits
#include "grammar/GrammarEngine.h" //mridu's engine + ShapeNode
#include "grammar/ShapeNode.h" //mridu's ShapeNode

namespace sg {

/**
 * OpenGL viewport, implemented as an FLTK Fl_Gl_Window subclass
 *   - all drawing happens in draw()
 *   - all interaction happens in handle()
 *   - state changes call redraw() to request a repaint
 * (https://www.fltk.org/doc-1.4/opengl.html)
 *
 * This class is ONLY rendering + input. It does not own the
 * buildings: it holds a reference to a SceneController (the scene model) and
 * asks it what to draw, what is selected, and where a dragged building may go.
 * Adding/editing/deleting buildings lives on the controller, not here.
 */
class SceneView : public Fl_Gl_Window {
public:
    // The view renders and edits the scene held by `controller`, which must
    // outlive the view (both are created in main() for the app's lifetime).
    SceneView(SceneController& controller, int x, int y, int w, int h,
              const char* label = nullptr);

    //render one frame...called whenever the window needs repainting
    void draw() override;

    //process a mouse/keyboard event
    //return 1 if consumed
    int handle(int event) override;

private:
    //one-time GL setup (lighting, depth test, clear color)
    void initGLState();

    //draw the reference grid on the y = 0 plane
    void drawGroundGrid();

    /**
     * Load one image file (PNG) into an OpenGL texture and remember it for a
     * given Material.
     * (call once.. after the GL context exists)
     */
    void loadTexture(Material material, const char* path);

    /**
     * Load every building material's texture. Called once from the first draw().
     */
    void loadTextures();

    /**
     * Draw the terminal shapes produced by the GrammarEngine
     */
    void drawGrammarScene();

    /**
     * Turn a 2D mouse pixel into a 3D selection ray that shoots from the camera into the scene
     *
     * params: mouseX,mouseY  cursor position in pixels
     *         outOrigin      (out) where the ray starts, in world space
     *         outDir         (out) unit-length direction the ray travels
     */
    void buildSelectionRay(int mouseX, int mouseY, V3& outOrigin, V3& outDir);

    /**
     * Test whether the selection ray passes through a terminal
     *
     * params: origin,dir  the ray (from buildSelectionRay)
     *         terminal    the terminal to test (uses its position, size, rotation)
     *         outT        (out) distance from the camera to terminal
     * returns true if the ray hits the terminal; use outT to find the closest hit
     */
    bool rayHitTerminal(const V3& origin, const V3& dir,
                         const ShapeNode& terminal, float& outT) const;

    /**
     * Determine what did the user click?
     * Builds the ray, tests it against every terminal, and keeps the closest hit
     * Selects the owning root of the closest terminal. Empty space deselects.
     */
    bool pickComponent(int mouseX, int mouseY);

    /**
     * Draw a highlight box around the whole selected building
     *
     * compute the building's bounding box and highlight it
     */
    void drawSelectionHighlight();

    /**
     * Intersect a ray with the flat ground plane (y = 0)
     * params: origin,dir  the ray (from buildSelectionRay)
     *         outPoint    (out) where the ray meets the ground
     * returns true if the ray actually meets the ground; false if it points up
     *         or runs parallel to the ground (so no valid hit point)
     */
    bool rayHitGround(const V3& origin, const V3& dir, V3& outPoint) const;

    // The scene model this view renders and edits. Owned by main(), not by us.
    // Selection, placement, and the grid/engine all live here now.
    SceneController& controller;

    Camera camera;
    bool glInitialized = false;
    bool texturesLoaded = false;

    // One GL texture id per material (Material enum values index this array: None=0, Wood=1, Glass=2, Stone=3)
    //0 means "no texture" -> use flat color
    unsigned int materialTextures[4] = {0, 0, 0, 0};

    //while a drag is in progress these track the move:
    bool movingBuilding = false; // true between grabbing and releasing the building

    //Offset between the building's placement and the ground point under the cursor at the moment of the grab
    //needed bc this stops the building from snapping its center to the cursor when you grab it off-center
    V3 grabOffset = V3(0, 0, 0);

    // Cached unit meshes... will build each primitive once and reuse it
    // each instance will be scaled via the modelview matrix
    Mesh boxMesh;
    Mesh cylinderMesh;

    //mouse vars for rotate/zoom controls
    int lastMouseX = 0;
    int lastMouseY = 0;
    bool dragging = false;
    bool panning = false;
};

} // namespace sg

#endif // VIEW_SCENEVIEW_H
