// 3D Shape Grammars: main GUI entry!
// main.cpp

#include <FL/Fl.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Box.H>

#include "view/SceneView.h"

int main(int argc, char** argv) {

    // window dimensions
    const int winW = 1024;
    const int winH = 720;

    const int toolbarH = 40; // top toolbar: reserved for preset buttons (office, church, etc)
    const int sidePanelW = 240; // side panel: reserved for tuning parameters and derivation tree

    Fl_Double_Window window(winW, winH, "3D Shape Grammars");
    window.begin();

    // top toolbar
    {
        Fl_Box* toolbar = new Fl_Box(0, 0, winW, toolbarH,
                                     "top toolbar: will contain presets");
        toolbar->box(FL_FLAT_BOX);
        toolbar->color(fl_rgb_color(45, 48, 56)); //dark grayish-blue
        toolbar->labelcolor(fl_rgb_color(150, 155, 165)); //lighter gray
        toolbar->labelsize(11);
    }

    //viewport for the scene
    const int viewX = 0;
    const int viewY = toolbarH;
    const int viewW = winW - sidePanelW;
    const int viewH = winH - toolbarH;
    sg::SceneView* view = new sg::SceneView(viewX, viewY, viewW, viewH); //see SceneView.h

    // side panel
    {
        Fl_Box* side = new Fl_Box(winW - sidePanelW, toolbarH, sidePanelW,
                                  winH - toolbarH,
                                  "side panel for parameters and \n derivation tree visualization");
        side->box(FL_FLAT_BOX);
        side->color(fl_rgb_color(38, 40, 47)); //slightly darker gray than top toolbar
        side->labelcolor(fl_rgb_color(150, 155, 165)); //same lighter gray as top toolbar font
        side->labelsize(11);
    }

    window.end();

    // viewport grows with the window, top toolbar and side panel stay the same
    window.resizable(view);

    window.show(argc, argv);
    return Fl::run();
}
