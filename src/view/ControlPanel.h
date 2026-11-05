// ControlPanel.h: the right-hand sidebar UI.
//
// Everything in the sidebar lives here, so main() only has to place the three
// top-level regions (toolbar, 3D view, this panel) and start FLTK. The panel:
//   - has the Add Office / Add Apartment preset buttons,
//   - edits the selected building's parameters (floors, bays, width, depth,
//     balconies) with validation, and applies / deletes,
//   - shows a status line and the derivation-tree panel (Checkpoint 3),
//   - keeps its fields and the tree in sync with the selected building.
//
// It drives everything through the shared SceneController; it never touches the
// GrammarEngine or the 3D view directly.

#ifndef VIEW_CONTROLPANEL_H
#define VIEW_CONTROLPANEL_H

#include <FL/Fl_Group.H>

#include <string>

#include "scene/SceneController.h"

// FLTK widget types used only as pointers inside the panel.
class Fl_Box;
class Fl_Button;
class Fl_Check_Button;
class Fl_Input;
class Fl_Int_Input;
class Fl_Float_Input;

namespace sg {

class DerivationTreeView;

/**
 * The sidebar, built as an Fl_Group. Construct it inside the main window after
 * the SceneController and SceneView exist; it wires its own button callbacks and
 * subscribes to the controller's selection changes. The `scene` reference must
 * outlive the panel (both live for the app's lifetime).
 */
class ControlPanel : public Fl_Group {
public:
    ControlPanel(SceneController& scene, int x, int y, int w, int h);

private:
    // --- actions (what the buttons/selection do) -------------------------

    // Add an Office (apartment=false) or Apartment (apartment=true) preset.
    void loadPreset(bool apartment);
    // Read + validate the fields and apply them to the selected building.
    void apply();
    // Delete the selected building.
    void deleteSelected();
    // Pull the selected building's family/parameters into the fields (or
    // disable them when nothing is selected), and refresh the tree.
    void syncSelection();
    // Show a one- or two-line status message (error = red).
    void message(const std::string& text, bool error = false);

    // --- FLTK callback trampolines (forward to the methods above) --------
    static void officeClicked(Fl_Widget*, void* self);
    static void apartmentClicked(Fl_Widget*, void* self);
    static void applyClicked(Fl_Widget*, void* self);
    static void deleteClicked(Fl_Widget*, void* self);

    SceneController& scene;

    // Widgets this panel reads/writes after construction.
    Fl_Box* heading = nullptr;
    Fl_Int_Input* floors = nullptr;
    Fl_Int_Input* bays = nullptr;
    Fl_Float_Input* width = nullptr;
    Fl_Float_Input* depth = nullptr;
    Fl_Check_Button* balconies = nullptr;
    Fl_Button* applyButton = nullptr;
    Fl_Button* deleteButton = nullptr;
    Fl_Box* status = nullptr;
    DerivationTreeView* tree = nullptr;

    // True when the selected building is an apartment (controls the balcony
    // field and the preset family used by apply()).
    bool apartmentActive = false;
};

} // namespace sg

#endif // VIEW_CONTROLPANEL_H
