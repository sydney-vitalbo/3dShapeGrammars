// ControlPanel.cpp
// The sidebar UI: preset buttons, parameter editing, status, and the derivation
// tree. Extracted from main.cpp so main() only lays out the top-level regions

#include "view/ControlPanel.h"
#include "view/DerivationTreeView.h"

#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Int_Input.H>
#include <FL/fl_draw.H>

#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <sstream>
#include <stdexcept>

#include "grammar/BuildingRules.h" // Parameters

namespace sg {

namespace {
// Sidebar palette (kept here so the panel owns its own look).
const Fl_Color ink   = fl_rgb_color(48, 52, 59);
const Fl_Color muted = fl_rgb_color(112, 116, 123);
const Fl_Color paper = fl_rgb_color(246, 247, 248);
const Fl_Color line  = fl_rgb_color(222, 224, 227);

// A quiet, one-pixel border for fields, without the default beveled edges.
constexpr Fl_Boxtype fieldBox = Fl_Boxtype(FL_FREE_BOXTYPE);
void drawFieldBox(int x, int y, int w, int h, Fl_Color color) {
    fl_color(color);
    fl_rectf(x, y, w, h);
    fl_color(fl_rgb_color(210, 212, 215));
    fl_rect(x, y, w, h);
}

// Register the custom field boxtype exactly once, however many panels exist.
void ensureFieldBoxType() {
    static bool registered = false;
    if (registered) return;
    Fl::set_boxtype(fieldBox, drawFieldBox, 8, 2, 16, 4);
    registered = true;
}

// Validate text before casting counts or passing dimensions to the rules.
// Empty, nonnumeric, fractional counts, and out-of-range values are rejected.
double readNumber(const Fl_Input& input, const char* name,
                  double minimum, double maximum, bool integer) {
    const char* text = input.value();
    char* end = nullptr;
    errno = 0;
    const double value = std::strtod(text, &end);
    const bool parsed = end != text;
    while (*end && std::isspace(static_cast<unsigned char>(*end))) ++end;
    if (!parsed || *end || errno == ERANGE || !std::isfinite(value) ||
        value < minimum || value > maximum || (integer && value != std::floor(value))) {
        throw std::invalid_argument(std::string(name) + " must be " +
            (integer ? "a whole number" : "a number") + " from " +
            std::to_string(int(minimum)) + " to " + std::to_string(int(maximum)) + ".");
    }
    return value;
}
} // namespace

ControlPanel::ControlPanel(SceneController& scene, int x, int y, int w, int h)
    : Fl_Group(x, y, w, h), scene(scene) {
    ensureFieldBoxType();
    box(FL_FLAT_BOX);
    color(paper);

    // All widgets are inset from the panel's left edge by a fixed margin.
    const int left = x + 24;

    // Keep preset selection and editing together in a fixed-width sidebar.
    auto* presetLabel = new Fl_Box(left, 80, 252, 18, "ADD BUILDING");
    presetLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    presetLabel->labelfont(FL_HELVETICA_BOLD);
    presetLabel->labelsize(11);
    presetLabel->labelcolor(muted);
    auto* office = new Fl_Button(left, 106, 122, 36, "Add Office");
    auto* apartment = new Fl_Button(left + 130, 106, 122, 36, "Add Apartment");
    for (Fl_Button* button : {office, apartment}) {
        button->color(FL_WHITE);
        button->labelcolor(ink);
        button->box(FL_FLAT_BOX);
        button->down_box(FL_FLAT_BOX);
        button->selection_color(ink);
        button->labelfont(FL_HELVETICA_BOLD);
        button->labelsize(13);
    }
    office->tooltip("Add an office building to the scene.");
    apartment->tooltip("Add an apartment building to the scene.");

    heading = new Fl_Box(left, 168, 252, 26, "Office parameters");
    heading->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    heading->labelfont(FL_HELVETICA_BOLD);
    heading->labelsize(17);
    heading->labelcolor(ink);
    auto* floorHint = new Fl_Box(left, 198, 252, 20, "Shared settings apply to each member");
    floorHint->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    floorHint->labelsize(12);
    floorHint->labelcolor(muted);

    floors = new Fl_Int_Input(left, 256, 120, 36, "Floors (1-12)");
    floors->tooltip("Includes the ground floor. Adding floors preserves each floor's height.");
    bays = new Fl_Int_Input(left + 132, 256, 120, 36, "Upper bays (2-8)");
    bays->tooltip("Repeated facade sections per upper floor. Ground-floor windows adapt around the entrance.");
    width = new Fl_Float_Input(left, 336, 120, 36, "Width (4-12)");
    depth = new Fl_Float_Input(left + 132, 336, 120, 36, "Depth (2-8)");
    floorHeight = new Fl_Float_Input(left, 402, 120, 30, "Floor height (1.2-3)");
    for (Fl_Input* input : std::initializer_list<Fl_Input*>{floors, bays, width, depth,floorHeight}) {
        input->box(fieldBox);
        input->color(FL_WHITE);
        input->labelcolor(muted);
        input->labelsize(12);
        input->align(FL_ALIGN_TOP_LEFT);
        input->textfont(FL_HELVETICA);
        input->textsize(15);
        input->textcolor(ink);
        input->cursor_color(ink);
        input->selection_color(ink);
    }
    balconies = new Fl_Check_Button(left+128, 402, 124, 28, "Balconies");
    balconies->color(paper);
    balconies->down_box(fieldBox);
    balconies->selection_color(ink);
    balconies->labelcolor(ink);
    balconies->labelsize(13);
    balconies->tooltip("Enable or disable upper-front apartment balconies, then click Apply changes. Joined/hidden bays stay hidden.");

    applyButton = new Fl_Button(left, 438, 252, 40, "Apply changes");
    applyButton->box(FL_FLAT_BOX);
    applyButton->down_box(FL_FLAT_BOX);
    applyButton->color(ink);
    applyButton->selection_color(fl_rgb_color(75, 80, 88));
    applyButton->labelcolor(FL_WHITE);
    applyButton->labelfont(FL_HELVETICA_BOLD);
    applyButton->labelsize(13);
    applyButton->shortcut(FL_CTRL + FL_Enter);
    deleteButton = new Fl_Button(left, 488, 122, 32, "Delete selected");
    detachButton = new Fl_Button(left+130,488,122,32,"Detach member");
    detachButton->tooltip("Click a member or its descendant in the derivation tree, then detach it from the unit.");
    deleteButton->box(fieldBox);
    deleteButton->down_box(FL_FLAT_BOX);
    deleteButton->color(paper);
    deleteButton->selection_color(line);
    deleteButton->labelcolor(ink);
    deleteButton->labelsize(12);
    deleteButton->tooltip("Remove the selected building from the scene.");

    status = new Fl_Box(left, 534, 252, 66);
    status->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
    status->labelsize(12);
    auto* divider = new Fl_Box(FL_FLAT_BOX, left, 610, 252, 1, "");
    divider->color(line);
    auto* help = new Fl_Box(left, 628, 252, 68,
        "Click building to select / drag to move\nDrag empty space to orbit\nRight drag to pan / scroll to zoom");
    help->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE);
    help->labelcolor(muted);
    help->labelsize(12);

    // Derivation-tree panel: lists the selected building's tree; clicking a node
    // highlights its component in the 3D view.
    auto* treeDivider = new Fl_Box(FL_FLAT_BOX, left, 704, 252, 1, "");
    treeDivider->color(line);
    auto* treeLabel = new Fl_Box(left, 716, 252, 18, "DERIVATION TREE");
    treeLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    treeLabel->labelfont(FL_HELVETICA_BOLD);
    treeLabel->labelsize(11);
    treeLabel->labelcolor(muted);
    // Fill the rest of the panel height, leaving a small bottom margin.
    const int treeTop = 740;
    const int treeHeight = (y + h) - treeTop - 20;
    tree = new DerivationTreeView(scene, left, treeTop, 252, treeHeight);
    tree->box(fieldBox);
    tree->color(FL_WHITE);
    tree->labelsize(12);
    tree->tooltip("Select a building, then click a node to highlight its component in 3D.");

    // The panel is a fixed-width sidebar; the 3D view takes the stretch.
    resizable(nullptr);
    end();

    // Wire up the buttons (this = the data passed to each trampoline).
    office->callback(officeClicked, this);
    apartment->callback(apartmentClicked, this);
    applyButton->callback(applyClicked, this);
    deleteButton->callback(deleteClicked, this);
    detachButton->callback(detachClicked,this);

    // Follow the selection: when it changes (via sidebar OR a click in the 3D
    // view) -- and after add/edit/delete, which re-select -- refresh the fields
    // and the tree so both track the current building and its (maybe new) tree.
    scene.onSelectionChanged([this] { syncSelection(); });
    syncSelection();
}

void ControlPanel::message(const std::string& text, bool error) {
    status->copy_label(text.c_str());
    status->labelcolor(error ? fl_rgb_color(160, 35, 35) : fl_rgb_color(95, 99, 106));
    status->redraw();
}

void ControlPanel::syncSelection() {
    building_rules::Parameters parameters;
    const bool selected = scene.getSelectedBuilding(apartmentActive, parameters);
    Fl_Widget* fields[] = {floors, bays, width, depth, floorHeight, balconies, applyButton, deleteButton};
    for (Fl_Widget* field : fields) {
        if (selected) field->activate(); else field->deactivate();
    }
    // The tree always follows the current selection (empty when none).
    tree->refresh();
    bool widthLocked=false,depthLocked=false;
    const bool unit=scene.selectedUnitLocks(widthLocked,depthLocked);
    deleteButton->copy_label(unit ? "Delete unit" : "Delete selected");
    deleteButton->tooltip(unit ? "Delete the entire unit and all its members." : "Remove the selected building from the scene.");
    if(unit) detachButton->activate(); else detachButton->deactivate();
    if(widthLocked) width->deactivate();
    if(depthLocked) depth->deactivate();
    if (!selected) {
        heading->copy_label("Select a building");
        message("Click a building to edit it, or add a preset using the buttons above.");
        return;
    }
    auto setNumber = [](Fl_Input* input, double value) {
        std::ostringstream text;
        text << value;
        input->value(text.str().c_str());
    };
    setNumber(floors, parameters.floors);
    setNumber(bays, parameters.bays);
    setNumber(width, parameters.width);
    setNumber(depth, parameters.depth);
    setNumber(floorHeight,parameters.floorHeight);
    balconies->value(parameters.balconies);
    if (!apartmentActive) balconies->deactivate();
    const std::string name = std::string(unit ? "Joined unit #" : apartmentActive ? "Apartment #" : "Office #")
        + std::to_string(scene.selectedBuildingId()+1);
    heading->copy_label((name + " parameters").c_str());
    message(name + " selected.\nEdit fields, then Apply changes.");
}

void ControlPanel::loadPreset(bool apartment) {
    building_rules::Parameters parameters;
    parameters.floors = 3;
    parameters.bays = 3;
    parameters.width = 6;
    parameters.depth = 3;
    parameters.floorHeight = 1.5f;
    parameters.balconies = apartment;
    try {
        const auto count = scene.addBuilding(apartment, parameters);
        message(std::string(apartment ? "Apartment" : "Office") +
                " added.\n" + std::to_string(count) + " pieces.\nClick any building to edit it.");
    } catch (const std::exception& error) {
        message(error.what(), true);
    }
}

void ControlPanel::apply() {
    try {
        bool lockWidth=false,lockDepth=false;
        const bool unit=scene.selectedUnitLocks(lockWidth,lockDepth);
        building_rules::Parameters parameters;
        parameters.floors = int(readNumber(*floors, "Floors", 1, 12, true));
        parameters.bays = int(readNumber(*bays, "Upper-floor bays", 2, 8, true));
        parameters.width = float(readNumber(*width, "Width", 4, 12, false));
        parameters.depth = float(readNumber(*depth, "Depth", 2, 8, false));
        parameters.floorHeight = float(readNumber(*floorHeight,"Floor height",1.2,3,false));
        parameters.balconies = apartmentActive && balconies->value();
        const auto count = scene.setBuilding(apartmentActive, parameters);
        message(std::string(unit ? "Joined unit" : apartmentActive ? "Apartment" : "Office") +
                " updated.\n" + std::to_string(parameters.floors) + " floors, " +
                std::to_string(parameters.bays) + " upper-floor bays.\n" +
                std::to_string(count) + " pieces.");
    } catch (const std::exception& error) {
        message(error.what(), true);
    }
}

void ControlPanel::deleteSelected() {
    if (scene.deleteSelectedBuilding())
        message("Building deleted.\nSelect another building or add a preset.");
}

void ControlPanel::officeClicked(Fl_Widget*, void* self) {
    static_cast<ControlPanel*>(self)->loadPreset(false);
}
void ControlPanel::apartmentClicked(Fl_Widget*, void* self) {
    static_cast<ControlPanel*>(self)->loadPreset(true);
}
void ControlPanel::applyClicked(Fl_Widget*, void* self) {
    static_cast<ControlPanel*>(self)->apply();
}
void ControlPanel::deleteClicked(Fl_Widget*, void* self) {
    static_cast<ControlPanel*>(self)->deleteSelected();
}
void ControlPanel::detachClicked(Fl_Widget*, void* self) {
    auto* panel=static_cast<ControlPanel*>(self);
    if(panel->scene.detachHighlightedBuilding())
        panel->message("Member detached. Drag it away to separate the buildings.");
    else panel->message("Select a member in the derivation tree first.",true);
}

} // namespace sg
