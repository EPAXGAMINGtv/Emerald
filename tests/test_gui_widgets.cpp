#include "TestFramework.hpp"

#include "emrldgui/Window.h"
#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventManager.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/event/EventTextInput.h"
#include "emrldgui/input/Keyboard.h"
#include "emrldgui/input/Mouse.h"
#include "emrldgui/ui_component/components/ContextMenu.h"
#include "emrldgui/ui_component/components/Dialog.h"
#include "emrldgui/ui_component/components/Image.h"
#include "emrldgui/ui_component/components/MenuBar.h"
#include "emrldgui/ui_component/components/Panel.h"
#include "emrldgui/ui_component/components/RadioButton.h"
#include "emrldgui/ui_component/components/Text.h"
#include "emrldgui/ui_component/components/Toast.h"
#include "emrldgui/ui_component/components/ToggleButton.h"
#include "emrldgui/ui_component/components/Tooltip.h"

#include <SDL.h>

#include <string>

namespace {

void push_key(SDL_Window* window, SDL_Keycode key, SDL_Keymod mod = KMOD_NONE) {
    SDL_Event event{};
    event.type = SDL_KEYDOWN;
    event.key.windowID = SDL_GetWindowID(window);
    event.key.state = SDL_PRESSED;
    event.key.repeat = 0;
    event.key.keysym.sym = key;
    event.key.keysym.mod = mod;
    SDL_PushEvent(&event);
}

} // namespace

TEST(gui_toggle_and_radio_buttons) {
    emgui::Window window("toggle-radio", 600, 400, SDL_WINDOW_SHOWN);
    auto& ui = window.getUIComponentManager();

    auto& toggle = ui.addComponent<emgui::ToggleButton>("Auto", 20, 20, 140, 44);
    int toggle_changes = 0;
    toggle.onCheckedChanged([&toggle_changes](emgui::ToggleButton&) {
        ++toggle_changes;
    });
    CHECK(toggle.getComponentName() == "ToggleButton");
    CHECK(toggle.getText() == "Auto");
    CHECK(!toggle.isChecked());
    toggle.setChecked(true);
    CHECK(toggle.isChecked());
    CHECK(toggle_changes == 1);
    toggle.toggle();
    CHECK(!toggle.isChecked());
    CHECK(toggle_changes == 2);
    toggle.setChecked(true);
    CHECK(toggle.isChecked());
    CHECK(toggle_changes == 3);

    auto& first = ui.addComponent<emgui::RadioButton>("First", "mode", false, 20, 100);
    auto& second = ui.addComponent<emgui::RadioButton>("Second", "mode", false, 20, 140);
    int radio_changes = 0;
    first.onCheckedChanged([&radio_changes](emgui::RadioButton&) { ++radio_changes; });
    CHECK(first.getComponentName() == "RadioButton");
    CHECK(first.getGroup() == "mode");
    CHECK(first.getLabel() == "First");
    CHECK(!first.isChecked());
    CHECK(!second.isChecked());

    first.setChecked(true);
    CHECK(first.isChecked());
    CHECK(!second.isChecked());
    CHECK(radio_changes == 1);
    second.setChecked(true);
    CHECK(second.isChecked());
    CHECK(!first.isChecked());

    first.setLabel("Updated");
    CHECK(first.getLabel() == "Updated");
    first.setEnabled(false);
    CHECK(!first.isEnabled());
    first.setEnabled(true);
    CHECK(first.isEnabled());
    first.setGroup("other");
    CHECK(first.getGroup() == "other");
}

TEST(gui_menus_tooltips_dialogs_and_toasts) {
    emgui::Window window("overlays", 800, 600, SDL_WINDOW_SHOWN);
    auto& ui = window.getUIComponentManager();

    auto& menu = ui.addComponent<emgui::ContextMenu>();
    int action_count = 0;
    menu.addItem("Open", [&action_count](emgui::ContextMenu&) { ++action_count; });
    menu.addSeparator();
    menu.addSubmenu("More", nullptr);
    CHECK(menu.getComponentName() == "ContextMenu");
    CHECK(menu.getItems().size() == 3U);
    CHECK(menu.getItems()[0].text == "Open");
    CHECK(menu.getItems()[1].separator);
    CHECK(menu.getItems()[2].submenu == nullptr);
    menu.showAt(100, 100);
    CHECK(menu.isOpen());
    menu.close();
    CHECK(!menu.isOpen());
    menu.setParent(nullptr);
    CHECK(menu.getParent() == nullptr);

    auto& bar = ui.addComponent<emgui::MenuBar>(20, 20, 300, 40);
    bar.addMenu("File", &menu);
    CHECK(bar.getComponentName() == "MenuBar");
    CHECK(bar.getEntries().size() == 1U);
    CHECK(bar.getEntries()[0].label == "File");
    CHECK(bar.getEntries()[0].menu == &menu);

    auto& anchor = ui.addComponent<emgui::Button>("anchor", 20, 80, 120, 40);
    auto& tooltip = ui.addComponent<emgui::Tooltip>("Info");
    CHECK(tooltip.getComponentName() == "Tooltip");
    CHECK(tooltip.getText() == "Info");
    tooltip.setText("Details");
    CHECK(tooltip.getText() == "Details");
    tooltip.attachTo(&anchor);
    CHECK(tooltip.getAnchor() == &anchor);
    tooltip.show("Popup", 150, 150);
    tooltip.hide();

    auto& dialog = ui.addComponent<emgui::Dialog>();
    dialog.setTitle("Confirm");
    dialog.setMessage("Continue?");
    dialog.addButton("OK", [](emgui::Dialog&) {});
    dialog.addButton("Cancel", [](emgui::Dialog&) {});
    int closes = 0;
    dialog.onClose([&closes](emgui::Dialog&) { ++closes; });
    CHECK(dialog.getComponentName() == "Dialog");
    CHECK(dialog.getTitle() == "Confirm");
    CHECK(dialog.getMessage() == "Continue?");
    CHECK(dialog.getButtonCount() == 2U);
    CHECK(!dialog.isOpen());
    dialog.show();
    CHECK(dialog.isOpen());
    CHECK(ui.getModalComponent() == &dialog);
    dialog.setResult(7);
    CHECK(dialog.getResult() == 7);
    dialog.close();
    CHECK(!dialog.isOpen());
    CHECK(closes == 1);

    auto& box = ui.addComponent<emgui::MessageBox>(
        "Question", "Yes or no?", emgui::MessageBox::Buttons::YesNoCancel
    );
    CHECK(box.getTitle() == "Question");
    CHECK(box.getMessage() == "Yes or no?");
    CHECK(box.getButtonCount() == 3U);
    CHECK(emgui::MessageBox::ResultOk == 1);
    CHECK(emgui::MessageBox::ResultCancel == 0);
    CHECK(emgui::MessageBox::ResultYes == 1);
    CHECK(emgui::MessageBox::ResultNo == 0);

    auto& toasts = ui.addComponent<emgui::ToastManager>(20, 500, 320);
    CHECK(toasts.getComponentName() == "ToastManager");
    CHECK(toasts.getCount() == 0U);
    toasts.show("Saved");
    CHECK(toasts.getCount() == 1U);
    toasts.show("Info", "Details", 1000);
    CHECK(toasts.getCount() == 2U);
    toasts.clear();
    CHECK(toasts.getCount() == 0U);
}

TEST(gui_text_image_and_panel) {
    emgui::Window window("drawables", 600, 400, SDL_WINDOW_SHOWN);
    auto& ui = window.getUIComponentManager();

    auto& text = ui.addComponent<emgui::Text>("Hello", 20, 20);
    CHECK(text.getComponentName() == "Text");
    CHECK(text.getText() == "Hello");
    text.setText("Emerald GUI");
    CHECK(text.getText() == "Emerald GUI");
    CHECK(text.getColor().a == 255);
    CHECK(text.getFontFamily() == "LiberationSans");
    CHECK(text.getFontSize() == 20);
    CHECK(text.getFontStyle() == "normal");
    CHECK(text.getLayoutSize().x > 0);
    CHECK(text.getLayoutSize().y > 0);

    auto& image = ui.addComponent<emgui::Image>("", 20, 80, 120, 90);
    CHECK(image.getComponentName() == "Image");
    CHECK(image.getTexture() == nullptr);
    image.setTexturePath("res/textures/test.bmp");
    CHECK(image.getTexturePath() == "res/textures/test.bmp");
    image.setFit(emgui::Image::Fit::Contain);
    CHECK(image.getFit() == emgui::Image::Fit::Contain);
    image.setTint(SDL_Color{10, 20, 30, 255});
    CHECK(image.getTint().r == 10);
    image.setSrcRect(SDL_Rect{0, 0, 16, 16});
    image.clearSrcRect();
    CHECK(image.getLayoutSize().x == 120);
    CHECK(image.getLayoutSize().y == 90);

    auto& panel = ui.addComponent<emgui::Panel>(200, 20, 300, 200, "Details");
    CHECK(panel.getComponentName() == "Panel");
    CHECK(panel.getTitle() == "Details");
    panel.setTitle("Settings");
    CHECK(panel.getTitle() == "Settings");
    panel.setSize(320, 240);
    CHECK(panel.getWidth() == 320);
    CHECK(panel.getHeight() == 240);
    CHECK(panel.getContentSize().x > 0);
    CHECK(panel.getContentSize().y > 0);
}

TEST(gui_events_and_input_helpers) {
    emgui::Window window("events", 500, 400, SDL_WINDOW_SHOWN);
    SDL_Window* sdl_window = window.getWindow();
    emgui::EventManager& events = window.getEventManager();

    CHECK(emgui::EventManager::getForWindow(sdl_window) == &events);
    CHECK(events.getWindow() == sdl_window);
    CHECK(events.getWindowID() == SDL_GetWindowID(sdl_window));
    CHECK(events.getWindowID() != 0U);

    int keys = 0;
    SDL_Keycode last_key = SDLK_UNKNOWN;
    const emgui::EventManager::ListenerId key_listener =
        events.addEventListener<emgui::EventKeyboard>(
            [&keys, &last_key](emgui::EventKeyboard& event) {
                ++keys;
                last_key = event.getKey();
            }
        );
    CHECK(key_listener != emgui::EventManager::kInvalidListener);
    push_key(sdl_window, SDLK_a);
    window.updateWindow();
    CHECK(keys == 1);
    CHECK(last_key == SDLK_a);

    int motions = 0;
    SDL_Point last_position{0, 0};
    events.registerEvent(emgui::EventMouse(sdl_window));
    const emgui::EventManager::ListenerId mouse_listener =
        events.addEventListener<emgui::EventMouse>(
            [&motions, &last_position](emgui::EventMouse& event) {
                if (event.isMoved()) {
                    ++motions;
                    last_position = {event.getX(), event.getY()};
                }
            }
        );
    CHECK(mouse_listener != emgui::EventManager::kInvalidListener);

    SDL_Event motion{};
    motion.type = SDL_MOUSEMOTION;
    motion.motion.windowID = SDL_GetWindowID(sdl_window);
    motion.motion.x = 45;
    motion.motion.y = 75;
    SDL_PushEvent(&motion);
    window.updateWindow();
    CHECK(motions == 1);
    CHECK(last_position.x == 45);
    CHECK(last_position.y == 75);

    int text_inputs = 0;
    events.registerEvent(emgui::EventTextInput(sdl_window));
    const emgui::EventManager::ListenerId text_listener =
        events.addEventListener<emgui::EventTextInput>(
            [&text_inputs](emgui::EventTextInput& event) {
                if (event.getText() == "x") {
                    ++text_inputs;
                }
            }
        );
    CHECK(text_listener != emgui::EventManager::kInvalidListener);

    SDL_Event text_event{};
    text_event.type = SDL_TEXTINPUT;
    text_event.text.windowID = SDL_GetWindowID(sdl_window);
    text_event.text.text[0] = 'x';
    SDL_PushEvent(&text_event);
    window.updateWindow();
    CHECK(text_inputs == 1);

    emgui::Keyboard keyboard(window);
    emgui::Mouse mouse(window);
    push_key(sdl_window, SDLK_b);
    window.updateWindow();
    CHECK(keyboard.isDown(SDLK_b));
    CHECK(keyboard.isPressed(SDLK_b));
    keyboard.clearFrameState();
    CHECK(keyboard.isDown(SDLK_b));
    CHECK(!keyboard.isPressed(SDLK_b));

    SDL_PushEvent(&motion);
    window.updateWindow();
    CHECK(mouse.getX() == 45);
    CHECK(mouse.getY() == 75);
    mouse.clearFrameState();
    CHECK(mouse.getX() == 45);

    events.removeListener(key_listener);
    events.removeListener(mouse_listener);
    events.removeListener(text_listener);
    CHECK(events.getWindowID() == SDL_GetWindowID(sdl_window));
}
