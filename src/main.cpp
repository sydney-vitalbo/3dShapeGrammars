// Checkpoint 2: preset buttons, parameter fields, and regeneration callbacks.
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Int_Input.H>

#include "view/SceneView.h"

#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <stdexcept>
#include <string>
#include <sstream>

namespace {
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

// Parameter fields follow the selected root in the shared scene engine.
struct BuildingControls {
    sg::SceneView* view;
    Fl_Box* heading;
    Fl_Int_Input* floors;
    Fl_Int_Input* bays;
    Fl_Float_Input* width;
    Fl_Float_Input* depth;
    Fl_Check_Button* balconies;
    Fl_Button* applyButton;
    Fl_Box* status;
    bool apartmentActive = false;

    void message(const std::string& text, bool error = false) {
        status->copy_label(text.c_str());
        status->labelcolor(error ? fl_rgb_color(160, 35, 35) : fl_rgb_color(35, 75, 55));
        status->redraw();
    }

    void syncSelection() {
        building_rules::Parameters parameters;
        const bool selected = view->getSelectedBuilding(apartmentActive, parameters);
        Fl_Widget* fields[] = {floors, bays, width, depth, balconies, applyButton};
        for (Fl_Widget* field : fields) {
            if (selected) field->activate(); else field->deactivate();
        }
        if (!selected) {
            heading->copy_label("Select a building");
            message("Click a building to edit it, or add a preset from the toolbar.");
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
        balconies->value(parameters.balconies);
        if (!apartmentActive) balconies->deactivate();
        const std::string name = std::string(apartmentActive ? "Apartment #" : "Office #")
            + std::to_string(view->selectedBuildingId()+1);
        heading->copy_label((name + " parameters").c_str());
        message(name + " selected.\nEdit fields, then Apply changes.");
    }

    void loadPreset(bool isApartment) {
        building_rules::Parameters parameters;
        parameters.floors = 3;
        parameters.bays = 3;
        parameters.width = 6;
        parameters.depth = 3;
        parameters.floorHeight = 1.5f;
        parameters.balconies = isApartment;
        try {
            const auto count = view->addBuilding(isApartment, parameters);
            message(std::string(isApartment ? "Apartment" : "Office") +
                    " added.\n" + std::to_string(count) + " pieces.\nClick any building to edit it.");
        } catch (const std::exception& error) {
            message(error.what(), true);
        }
    }

    void apply() {
        try {
            building_rules::Parameters parameters;
            parameters.floors = int(readNumber(*floors, "Floors", 1, 12, true));
            parameters.bays = int(readNumber(*bays, "Upper-floor bays", 2, 8, true));
            parameters.width = float(readNumber(*width, "Width", 4, 12, false));
            parameters.depth = float(readNumber(*depth, "Depth", 2, 8, false));
            parameters.floorHeight = 1.5f; // Keep both families on the same floor grid.
            parameters.balconies = apartmentActive && balconies->value();
            const auto count = view->setBuilding(apartmentActive, parameters);
            message(std::string(apartmentActive ? "Apartment" : "Office") +
                    " updated.\n" + std::to_string(parameters.floors) + " floors, " +
                    std::to_string(parameters.bays) + " upper-floor bays.\n" +
                    std::to_string(count) + " pieces.");
        } catch (const std::exception& error) {
            message(error.what(), true);
        }
    }

    static void officeClicked(Fl_Widget*, void* data) {
        static_cast<BuildingControls*>(data)->loadPreset(false);
    }
    static void apartmentClicked(Fl_Widget*, void* data) {
        static_cast<BuildingControls*>(data)->loadPreset(true);
    }
    static void applyClicked(Fl_Widget*, void* data) {
        static_cast<BuildingControls*>(data)->apply();
    }
};
} // namespace

int main(int argc, char** argv) {
    const int winW = 1100, winH = 740, toolbarH = 52, sidePanelW = 280;
    const int panelX = winW - sidePanelW;
    Fl_Double_Window window(winW, winH, "3D Shape Grammars");
    window.begin();

    auto* toolbar = new Fl_Group(0, 0, winW, toolbarH);
    toolbar->box(FL_FLAT_BOX);
    toolbar->color(fl_rgb_color(45, 48, 56));
    auto* office = new Fl_Button(16, 10, 110, 32, "Add Office");
    auto* apartment = new Fl_Button(138, 10, 110, 32, "Add Apartment");
    for (Fl_Button* button : {office, apartment}) {
        button->selection_color(fl_rgb_color(180, 210, 230));
    }
    office->tooltip("Add an office building to the scene.");
    apartment->tooltip("Add an apartment building to the scene.");
    auto* toolbarHint = new Fl_Box(270, 10, 490, 32, "Add buildings, then click one to edit its parameters.");
    toolbarHint->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    toolbarHint->labelcolor(FL_WHITE);
    toolbarHint->labelsize(12);
    toolbar->resizable(nullptr);
    toolbar->end();

    auto* view = new sg::SceneView(0, toolbarH, panelX, winH - toolbarH);
    auto* panel = new Fl_Group(panelX, toolbarH, sidePanelW, winH - toolbarH);
    panel->box(FL_FLAT_BOX);
    panel->color(fl_rgb_color(235, 237, 240));
    auto* heading = new Fl_Box(panelX + 20, 76, 240, 28, "Office parameters");
    heading->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    heading->labelfont(FL_BOLD);
    auto* floorHint = new Fl_Box(panelX + 20, 108, 240, 22, "Floor height: 1.5 units (fixed)");
    floorHint->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    floorHint->labelsize(12);

    const int fieldX = winW - 112;
    auto* floors = new Fl_Int_Input(fieldX, 152, 92, 30, "Floors (1-12)");
    floors->tooltip("Includes the ground floor. Adding floors preserves each floor's height.");
    auto* bays = new Fl_Int_Input(fieldX, 206, 92, 30, "Upper bays (2-8)");
    bays->tooltip("Repeated facade sections per upper floor. Ground-floor windows adapt around the entrance.");
    auto* width = new Fl_Float_Input(fieldX, 260, 92, 30, "Width (4-12)");
    auto* depth = new Fl_Float_Input(fieldX, 314, 92, 30, "Depth (2-8)");
    auto* balconies = new Fl_Check_Button(panelX + 20, 368, 240, 28, "Apartment balconies");
    balconies->tooltip("Balconies appear only on upper front apartment floors.");
    auto* apply = new Fl_Button(panelX + 20, 420, 240, 36, "Apply changes");
    apply->shortcut(FL_CTRL + FL_Enter);
    auto* status = new Fl_Box(panelX + 20, 478, 240, 110);
    status->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
    status->labelsize(13);
    auto* help = new Fl_Box(panelX + 20, 620, 240, 76,
        "Click building: select\nDrag building: move on grid\nDrag empty space: orbit\nRight drag: pan | Scroll: zoom");
    help->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE);
    help->labelsize(12);
    panel->resizable(nullptr);
    panel->end();

    BuildingControls controls{view, heading, floors, bays,
                              width, depth, balconies, apply, status};
    office->callback(BuildingControls::officeClicked, &controls);
    apartment->callback(BuildingControls::apartmentClicked, &controls);
    apply->callback(BuildingControls::applyClicked, &controls);
    view->onSelectionChanged([&controls] { controls.syncSelection(); });
    controls.syncSelection();
    controls.loadPreset(false);

    window.end();
    window.resizable(view);
    window.size_range(1000, 700);
    window.show(argc, argv);
    return Fl::run();
}
