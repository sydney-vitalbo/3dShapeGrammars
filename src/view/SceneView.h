// SceneView.h: the actual 3D viewport the user sees and interacts with

#ifndef VIEW_SCENEVIEW_H
#define VIEW_SCENEVIEW_H

#include <FL/Fl_Gl_Window.H>

#include "scene/Camera.h"
#include "scene/Mesh.h"
#include "scene/Primitives.h"
#include "grammar/GrammarEngine.h" //mridu's engine + ShapeNode
#include "grammar/ShapeNode.h" //mridu's ShapeNode
#include "grammar/Rules.h" //mridu's rules
#include "grammar/BuildingRules.h" //borna's building rules

namespace sg {

/**
 * OpenGL viewport, implemented as an FLTK Fl_Gl_Window subclass
 *   - all drawing happens in draw()
 *   - all interaction happens in handle()
 *   - state changes call redraw() to request a repaint
 * (https://www.fltk.org/doc-1.4/opengl.html)
 */
class SceneView : public Fl_Gl_Window {
public:
    SceneView(int x, int y, int w, int h, const char* label = nullptr);

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
     * Returns true if building was hit...change once buildings have ids
     */
    bool pickComponent(int mouseX, int mouseY);

    /**
     * Draw a highlight box around the whole selected building
     *
     * compute the building's bounding box and highlight it
     */
    void drawSelectionHighlight();

    Camera camera;
    bool glInitialized = false;

    //selection state (just a flag right now...change once buildings have ids)
    bool buildingSelected = false;

    // Cached unit meshes... will build each primitive once and reuse it
    // each instance will be scaled via the modelview matrix
    Mesh boxMesh;
    Mesh cylinderMesh;

    primitives::GroundGrid grid;

    //generate a building once and render its terminals every frame (then regenerate when parameters change)
    GrammarEngine engine;

    //mouse vars for rotate/zoom controls
    int lastMouseX = 0;
    int lastMouseY = 0;
    bool dragging = false;
    bool panning = false;
};

} // namespace sg

#endif // VIEW_SCENEVIEW_H
