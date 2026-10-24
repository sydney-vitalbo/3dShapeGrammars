// SceneView.h: the actual 3D viewport the user sees and interacts with
// Sydney Vitalbo

#ifndef VIEW_SCENEVIEW_H
#define VIEW_SCENEVIEW_H

#include <FL/Fl_Gl_Window.H>

#include "scene/Camera.h"
#include "scene/Mesh.h"
#include "scene/Primitives.h"

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
     * Checkpoint 1 placeholder scene: one box and one cylinder
     *
     * LATER: switch to traversing a ShapeNode tree (composing parent/child transforms and drawing terminal
     * primitives)
     */
    void drawDemoScene();

    Camera camera;
    bool glInitialized = false;

    // Cached unit meshes... will build each primitive once and reuse it
    // each instance will be scaled via the modelview matrix
    Mesh boxMesh;
    Mesh cylinderMesh;

    primitives::GroundGrid grid;

    //mouse vars for rotate/zoom controls
    int lastMouseX = 0;
    int lastMouseY = 0;
    bool dragging = false;
    bool panning = false;
};

} // namespace sg

#endif // VIEW_SCENEVIEW_H
