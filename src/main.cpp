// Checkpoint 2: preset buttons, parameter fields, and regeneration callbacks.
#include <FL/Fl.H>
#include <FL/Fl_Box.H>
#include <FL/Fl_Button.H>
#include <FL/Fl_Check_Button.H>
#include <FL/Fl_Double_Window.H>
#include <FL/Fl_Float_Input.H>
#include <FL/Fl_Group.H>
#include <FL/Fl_Int_Input.H>
#include <FL/fl_draw.H>

#include "view/SceneView.h"

#include <cerrno>
#include <cctype>
#include <cmath>
#include <cstdlib>
#include <initializer_list>
#include <stdexcept>
#include <string>

namespace {
// A quiet, one-pixel border for fields, without the default beveled edges.
constexpr Fl_Boxtype fieldBox = Fl_Boxtype(FL_FREE_BOXTYPE);
void drawFieldBox(int x, int y, int w, int h, Fl_Color color) {
    fl_color(color);
    fl_rectf(x, y, w, h);
    fl_color(fl_rgb_color(210, 212, 215));
    fl_rect(x, y, w, h);
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

// UI state for the current building. Building IDs/multiple instances come later.
// The callbacks only select parameters; SceneView delegates expansion to the engine.
struct BuildingControls {
    sg::SceneView* view;
    Fl_Button* office;
    Fl_Button* apartment;
    Fl_Box* heading;
    Fl_Int_Input* floors;
    Fl_Int_Input* bays;
    Fl_Float_Input* width;
    Fl_Float_Input* depth;
    Fl_Check_Button* balconies;
    Fl_Box* status;
    bool apartmentActive = false;

    void message(const std::string& text, bool error = false) {
        status->copy_label(text.c_str());
        status->labelcolor(error ? fl_rgb_color(160, 35, 35) : fl_rgb_color(95, 99, 106));
        status->redraw();
    }

    void updatePresetButtons() {
        office->value(!apartmentActive);
        apartment->value(apartmentActive);
        for (Fl_Button* button : {office, apartment}) {
            button->color(button->value() ? fl_rgb_color(48, 52, 59) : FL_WHITE);
            button->labelcolor(button->value() ? FL_WHITE : fl_rgb_color(65, 69, 76));
            button->redraw();
        }
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
            const auto count = view->setBuilding(isApartment, parameters);
            apartmentActive = isApartment;
            floors->value("3"); bays->value("3");
            width->value("6"); depth->value("3");
            balconies->value(isApartment);
            if (isApartment) balconies->activate(); else balconies->deactivate();
            heading->copy_label(isApartment ? "Apartment parameters" : "Office parameters");
            message(std::string(isApartment ? "Apartment" : "Office") +
                    " preset loaded.\n" + std::to_string(count) + " pieces.\nEdit fields, then Apply changes.");
        } catch (const std::exception& error) {
            message(error.what(), true);
        }
        updatePresetButtons();
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
    Fl::set_boxtype(fieldBox, drawFieldBox, 8, 2, 16, 4);
    const Fl_Color ink = fl_rgb_color(48, 52, 59);
    const Fl_Color muted = fl_rgb_color(112, 116, 123);
    const Fl_Color paper = fl_rgb_color(246, 247, 248);
    const Fl_Color line = fl_rgb_color(222, 224, 227);
    const int winW = 1100, winH = 740, toolbarH = 60, sidePanelW = 300;
    const int panelX = winW - sidePanelW, left = panelX + 24;
    Fl_Double_Window window(winW, winH, "3D Shape Grammars");
    window.color(paper);
    window.begin();

    auto* toolbar = new Fl_Group(0, 0, winW, toolbarH);
    toolbar->box(FL_FLAT_BOX);
    toolbar->color(FL_WHITE);
    auto* title = new Fl_Box(24, 10, 260, 22, "3D Shape Grammars");
    title->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    title->labelfont(FL_HELVETICA_BOLD);
    title->labelsize(17);
    title->labelcolor(ink);
    auto* subtitle = new Fl_Box(24, 33, 400, 17, "Choose a building. Adjust its parameters. Explore in 3D.");
    subtitle->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    subtitle->labelsize(12);
    subtitle->labelcolor(muted);
    auto* toolbarLine = new Fl_Box(FL_FLAT_BOX, 0, toolbarH - 1, winW, 1, "");
    toolbarLine->color(line);
    toolbar->resizable(nullptr);
    toolbar->end();

    auto* view = new sg::SceneView(0, toolbarH, panelX, winH - toolbarH);
    auto* panel = new Fl_Group(panelX, toolbarH, sidePanelW, winH - toolbarH);
    panel->box(FL_FLAT_BOX);
    panel->color(paper);
    // Keep preset selection and editing together in a fixed-width sidebar.
    auto* presetLabel = new Fl_Box(left, 80, 252, 18, "BUILDING PRESET");
    presetLabel->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    presetLabel->labelfont(FL_HELVETICA_BOLD);
    presetLabel->labelsize(11);
    presetLabel->labelcolor(muted);
    auto* office = new Fl_Button(left, 106, 122, 36, "Office");
    auto* apartment = new Fl_Button(left + 130, 106, 122, 36, "Apartment");
    for (Fl_Button* button : {office, apartment}) {
        button->type(FL_RADIO_BUTTON);
        button->box(FL_FLAT_BOX);
        button->down_box(FL_FLAT_BOX);
        button->selection_color(ink);
        button->labelfont(FL_HELVETICA_BOLD);
        button->labelsize(13);
    }
    office->tooltip("Replace the current model with the default office preset.");
    apartment->tooltip("Replace the current model with the default apartment preset.");
    auto* heading = new Fl_Box(left, 168, 252, 26, "Office parameters");
    heading->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    heading->labelfont(FL_HELVETICA_BOLD);
    heading->labelsize(17);
    heading->labelcolor(ink);
    auto* floorHint = new Fl_Box(left, 198, 252, 20, "Floor height fixed at 1.5 units");
    floorHint->align(FL_ALIGN_LEFT | FL_ALIGN_INSIDE);
    floorHint->labelsize(12);
    floorHint->labelcolor(muted);

    auto* floors = new Fl_Int_Input(left, 256, 120, 36, "Floors (1-12)");
    floors->tooltip("Includes the ground floor. Adding floors preserves each floor's height.");
    auto* bays = new Fl_Int_Input(left + 132, 256, 120, 36, "Upper bays (2-8)");
    bays->tooltip("Repeated facade sections per upper floor. Ground-floor windows adapt around the entrance.");
    auto* width = new Fl_Float_Input(left, 336, 120, 36, "Width (4-12)");
    auto* depth = new Fl_Float_Input(left + 132, 336, 120, 36, "Depth (2-8)");
    for (Fl_Input* input : std::initializer_list<Fl_Input*>{floors, bays, width, depth}) {
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
    auto* balconies = new Fl_Check_Button(left, 392, 252, 28, "Apartment balconies");
    balconies->color(paper);
    balconies->down_box(fieldBox);
    balconies->selection_color(ink);
    balconies->labelcolor(ink);
    balconies->labelsize(13);
    balconies->tooltip("Balconies appear only on upper front apartment floors.");
    auto* apply = new Fl_Button(left, 438, 252, 40, "Apply changes");
    apply->box(FL_FLAT_BOX);
    apply->down_box(FL_FLAT_BOX);
    apply->color(ink);
    apply->selection_color(fl_rgb_color(75, 80, 88));
    apply->labelcolor(FL_WHITE);
    apply->labelfont(FL_HELVETICA_BOLD);
    apply->labelsize(13);
    apply->shortcut(FL_CTRL + FL_Enter);
    auto* status = new Fl_Box(left, 500, 252, 94);
    status->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE | FL_ALIGN_WRAP);
    status->labelsize(12);
    auto* divider = new Fl_Box(FL_FLAT_BOX, left, 610, 252, 1, "");
    divider->color(line);
    auto* help = new Fl_Box(left, 628, 252, 68,
        "Click building to select / drag to move\nDrag empty space to orbit\nRight drag to pan / scroll to zoom");
    help->align(FL_ALIGN_LEFT | FL_ALIGN_TOP | FL_ALIGN_INSIDE);
    help->labelcolor(muted);
    help->labelsize(12);
    panel->resizable(nullptr);
    panel->end();

    BuildingControls controls{view, office, apartment, heading, floors, bays,
                              width, depth, balconies, status};
    office->callback(BuildingControls::officeClicked, &controls);
    apartment->callback(BuildingControls::apartmentClicked, &controls);
    apply->callback(BuildingControls::applyClicked, &controls);
    controls.loadPreset(false);

    window.end();
    window.resizable(view);
    window.size_range(1000, 700);
    window.show(argc, argv);
    return Fl::run();
}
