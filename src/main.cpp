// main.cpp: app entry point and top-level window layout.
//
// main lays out the three regions of the window and
// starts FLTK. Everything else lives in its own unit
//   - the 3D viewport        -> SceneView
//   - the scene/building model -> SceneController
//   - the whole right sidebar  -> ControlPanel (preset buttons, parameter
//                                 editing, status, derivation tree)

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Group.H>

#include "view/SceneView.h"
#include "view/ControlPanel.h"
#include "scene/SceneController.h"

namespace {
// Build the top title bar across the window. Purely decorative; no callbacks.
void buildToolbar(int winW, int toolbarH) {
    const Fl_Color ink = fl_rgb_color(48, 52, 59);
    const Fl_Color muted = fl_rgb_color(112, 116, 123);
    const Fl_Color line = fl_rgb_color(222, 224, 227);

    auto* toolbar = new Fl_Group(0, 0, winW, toolbarH);
    toolbar->box(FL_FLAT_BOX);
    toolbar->color(FL_WHITE);
    auto* title = new Fl_Box(24, 10, 260, 22, "3D Shape Grammars");
    title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    title->labelfont(FL_HELVETICA_BOLD);
    title->labelsize(17);
    title->labelcolor(ink);
    auto* subtitle = new Fl_Box(24, 33, 400, 17, "Add buildings. Select one to edit. Explore in 3D.");
    subtitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    subtitle->labelsize(12);
    subtitle->labelcolor(muted);
    auto* toolbarLine = new Fl_Box(FL_FLAT_BOX, 0, toolbarH - 1, winW, 1, "");
    toolbarLine->color(line);
    toolbar->resizable(nullptr);
    toolbar->end();
}
} // namespace

int main(int argc, char** argv) {
    const Fl_Color paper = fl_rgb_color(246, 247, 248);
    // Tall enough for the sidebar's editing controls + derivation-tree panel.
    const int winW = 1100, winH = 880, toolbarH = 60, sidePanelW = 300;
    const int panelX = winW - sidePanelW;

    Fl_Double_Window window(winW, winH, "3D Shape Grammars");
    window.color(paper);
    window.begin();

    buildToolbar(winW, toolbarH);

    // The scene model (buildings + placement + selection) is shared by the 3D
    // view (renders/edits it) and the sidebar (adds/edits). It outlives both.
    static sg::SceneController scene;

    // Three regions below the toolbar: the 3D view fills the left, the sidebar
    // is a fixed-width column on the right. The ControlPanel builds its own
    // contents and wires its own callbacks + selection sync.
    auto* view = new sg::SceneView(scene, 0, toolbarH, panelX, winH - toolbarH);
    new sg::ControlPanel(scene, panelX, toolbarH, sidePanelW, winH - toolbarH);

    window.end();
    window.resizable(view);            // only the 3D view stretches on resize
    window.size_range(1000, 820);      // keep the sidebar + tree fully visible
    window.show(argc, argv);
    return Fl::run();
}
