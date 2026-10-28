// SceneView.cpp
// Sydney Vitalbo

#include "view/SceneView.h"

#include <FL/Fl.H>
#include <FL/gl.h>

#include <cstdlib> // std::abs

//macOS compatibility
#ifdef __APPLE__
#  include <OpenGL/gl.h>
#  include <OpenGL/glu.h>
#else
#  include <GL/gl.h>
#  include <GL/glu.h>
#endif

namespace sg {

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

    //ask the engine to generate a building preset
    engine.GenerateTest();
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
        //map the grammar Material tag to an RGB tint for this terminal
        float r = 0.8f, g = 0.8f, b = 0.8f;
        switch (t.material) {
            case Material::Stone: r = 0.75f; g = 0.79f; b = 0.82f; break; // light gray stone
            case Material::Glass: r = 0.20f; g = 0.56f; b = 0.68f; break; // blue-teal glass
            case Material::Wood:  r = 0.18f; g = 0.25f; b = 0.30f; break; // dark trim
            case Material::None:  r = 0.19f; g = 0.22f; b = 0.24f; break; // roof deck
        }
        glColor3f(r, g, b);

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
    }
}

// void SceneView::drawDemoScene() {
//     //meshes are unit-sized and centered at the origin
//     //to place a copy somewhere, temporarily change openGL's "modelview" transform, draw, then undo the change:
//     //   glPushMatrix() : save the current transform
//     //   glTranslatef() : move the origin for whatever we draw next
//     //   glScalef()     : resize whatever we draw next
//     //   <draw>
//     //   glPopMatrix()  : restore the saved transform
//     //https://www.scribd.com/document/135598965/NeHe-OpenGL-Tutorial#page=9

//     // openGL applies these to the object in reverse! so scale first then translate

//     //cube
//     glPushMatrix();
//     glTranslatef(-2.0f, 1.5f, 0.0f); // put it left of center; y=1.5 so its base sits on the ground (y=0)
//     glScalef(2.0f, 3.0f, 2.0f);      // 2 wide, 3 tall, 2 deep (unit box was 1x1x1)
//     glColor3f(0.80f, 0.55f, 0.35f);  // warm orange tint (this is the material color)
//     boxMesh.draw();
//     glPopMatrix();

//     //cylinder
//     glPushMatrix();
//     glTranslatef(2.0f, 1.5f, 0.0f);  // right of center, base on the ground
//     glScalef(1.6f, 3.0f, 1.6f);
//     glColor3f(0.45f, 0.65f, 0.85f);  // cool blue tint
//     cylinderMesh.draw();
//     glPopMatrix();
// }

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
            dragging = true;
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

        if (dragging) {
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
