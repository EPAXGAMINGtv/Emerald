#include "TestFramework.hpp"

#include "emrldgui/EmeraldGUI.h"
#include "emrldgui/Window.h"
#include "emrldgui/ui_component/components/Button.h"
#include "emrldgui/ui_component/components/CheckBox.h"
#include "emrldgui/ui_component/components/ComboBox.h"
#include "emrldgui/ui_component/components/ListView.h"
#include "emrldgui/ui_component/components/ProgressBar.h"
#include "emrldgui/ui_component/components/Slider.h"
#include "emrldgui/ui_component/components/Tabs.h"
#include "emrldgui/ui_component/components/TextArea.h"
#include "emrldgui/ui_component/components/TextField.h"

#include <SDL.h>

#include <string>
#include <vector>

TEST(gui_window_and_component_manager) {
    emgui::Window window("core-window", 640, 480, SDL_WINDOW_SHOWN);
    auto& ui = window.getUIComponentManager();

    CHECK(window.getTitle() == "core-window");
    CHECK(window.getWidth() == 640);
    CHECK(window.getHeight() == 480);
    CHECK(window.getWindow() != nullptr);
    CHECK(ui.getWindow() == &window);
    CHECK(ui.getComponentCount() == 0U);

    auto& button = ui.addComponent<emgui::Button>("Save", 10, 20, 120, 40);
    CHECK(ui.getComponentCount() == 1U);
    CHECK(ui.contains(&button));
    CHECK(button.getWindow() == &window);
    CHECK(button.getComponentName() == "Button");
    CHECK(button.getPosition().x == 10);
    CHECK(button.getPosition().y == 20);

    button.setPosition(15, 25);
    CHECK(button.getPosition().x == 15);
    CHECK(button.getPosition().y == 25);
    button.setStyleClass("primary");
    CHECK(button.getStyleClass() == "primary");
    button.setStyleId("save-button");
    CHECK(button.getStyleId() == "save-button");
    button.setVisible(false);
    CHECK(!button.isVisible());
    button.setVisible(true);
    CHECK(button.isVisible());

    ui.setFocusedComponent(&button);
    CHECK(ui.getFocusedComponent() == &button);
    ui.setModalComponent(&button);
    CHECK(ui.getModalComponent() == &button);
    ui.setModalComponent(nullptr);

    window.setTitle("renamed");
    CHECK(window.getTitle() == "renamed");
    window.setWidth(800);
    window.setHeight(600);
    CHECK(window.getWidth() == 800);
    CHECK(window.getHeight() == 600);
    CHECK(!window.shouldClose());

    window.updateWindow();
    window.renderUI();
    CHECK(ui.getComponentCount() == 1U);

    ui.removeComponent(&button);
    CHECK(ui.getComponentCount() == 0U);
    CHECK(!ui.contains(&button));
}

TEST(gui_button_and_checkbox_state) {
    emgui::Window window("core-buttons", 500, 400, SDL_WINDOW_SHOWN);
    auto& ui = window.getUIComponentManager();

    auto& button = ui.addComponent<emgui::Button>("OK", 20, 20, 160, 40);
    CHECK(button.getText() == "OK");
    button.setText("Apply");
    CHECK(button.getText() == "Apply");

    button.setButtonType(emgui::Button::ButtonType::Primary);
    CHECK(button.getButtonType() == emgui::Button::ButtonType::Primary);
    button.setButtonType(emgui::Button::ButtonType::Danger);
    CHECK(button.getButtonType() == emgui::Button::ButtonType::Danger);
    button.setSize(120, 44);
    CHECK(button.getWidth() == 120);
    CHECK(button.getHeight() == 44);
    CHECK(button.isEnabled());
    CHECK(!button.isHovered());
    CHECK(!button.isPressed());
    CHECK(button.getPressProgress() == 0.0f);
    button.setEnabled(false);
    CHECK(!button.isEnabled());
    button.setEnabled(true);
    CHECK(button.isEnabled());

    auto& box = ui.addComponent<emgui::CheckBox>("Option", false, 10, 100);
    int changes = 0;
    box.onCheckedChanged([&changes](emgui::CheckBox&) { ++changes; });
    CHECK(box.getComponentName() == "CheckBox");
    CHECK(box.getLabel() == "Option");
    CHECK(!box.isChecked());
    CHECK(changes == 0);

    box.setChecked(true);
    CHECK(box.isChecked());
    CHECK(box.getStyleClass() == "checked");
    CHECK(changes == 1);
    box.setChecked(true);
    CHECK(changes == 1);

    box.toggle();
    CHECK(!box.isChecked());
    CHECK(changes == 2);
    box.setLabel("Enabled");
    CHECK(box.getLabel() == "Enabled");
    box.setEnabled(false);
    CHECK(!box.isEnabled());
    box.setEnabled(true);
    CHECK(box.isEnabled());
}

TEST(gui_text_inputs) {
    emgui::Window window("core-text", 600, 500, SDL_WINDOW_SHOWN);
    auto& ui = window.getUIComponentManager();

    auto& field = ui.addComponent<emgui::TextField>("hint", 20, 20, 200, 40);
    int field_changes = 0;
    field.onTextChanged([&field_changes](emgui::TextField&) { ++field_changes; });
    CHECK(field.getComponentName() == "TextField");
    CHECK(field.getPlaceholder() == "hint");
    CHECK(field.getText().empty());

    field.setText("Emerald");
    CHECK(field.getText() == "Emerald");
    CHECK(field_changes == 1);
    field.selectAll();
    CHECK(field.hasSelection());
    CHECK(field.getSelectedText() == "Emerald");
    CHECK(field.getSelectionStart() == 0U);
    CHECK(field.getSelectionEnd() == 7U);
    field.clearSelection();
    CHECK(!field.hasSelection());

    field.setPlaceholder("name");
    CHECK(field.getPlaceholder() == "name");
    field.setSize(240, 60);
    CHECK(field.getWidth() == 240);
    CHECK(field.getHeight() == 60);
    field.setEnabled(false);
    CHECK(!field.isEnabled());
    field.setEnabled(true);
    CHECK(field.isEnabled());

    auto& area = ui.addComponent<emgui::TextArea>("notes", 20, 90, 320, 160);
    int area_changes = 0;
    area.onTextChanged([&area_changes](emgui::TextArea&) { ++area_changes; });
    CHECK(area.getComponentName() == "TextArea");
    CHECK(area.getPlaceholder() == "notes");
    area.setText("first\nsecond\nthird");
    CHECK(area.getText() == "first\nsecond\nthird");
    CHECK(area.getLineCount() == 3U);
    CHECK(area_changes == 1);
    CHECK(area.getCaretLine() == 2U);
    CHECK(area.getCaretColumn() == 5U);

    area.setReadOnly(true);
    CHECK(area.isReadOnly());
    area.setReadOnly(false);
    CHECK(!area.isReadOnly());
    area.setEnabled(false);
    CHECK(!area.isEnabled());
    area.setEnabled(true);
    CHECK(area.isEnabled());
    area.setSize(-10, -10);
    CHECK(area.getLayoutSize().x == 0);
    CHECK(area.getLayoutSize().y == 0);
}

TEST(gui_selection_components) {
    emgui::Window window("core-selection", 700, 500, SDL_WINDOW_SHOWN);
    auto& ui = window.getUIComponentManager();

    auto& list = ui.addComponent<emgui::ListView>(20, 20, 250, 200);
    list.setItems({"Alpha", "Beta", "Gamma"});
    CHECK(list.getComponentName() == "ListView");
    CHECK(list.getItems().size() == 3U);
    list.setSelectedIndex(1);
    CHECK(list.getSelectedIndex() == 1);
    CHECK(list.getSelectedText() == "Beta");
    list.setSelectedIndex(99);
    CHECK(list.getSelectedIndex() == -1);
    CHECK(list.getSelectedText().empty());

    auto& combo = ui.addComponent<emgui::ComboBox>(
        std::vector<std::string>{"One", "Two", "Three"}, 0, 300, 20, 200, 48
    );
    CHECK(combo.getComponentName() == "ComboBox");
    CHECK(combo.getItems().size() == 3U);
    CHECK(combo.getSelectedIndex() == 0);
    CHECK(combo.getSelectedText() == "One");
    combo.setSelectedIndex(2);
    CHECK(combo.getSelectedIndex() == 2);
    CHECK(combo.getSelectedText() == "Three");
    combo.setOpen(true);
    CHECK(combo.isOpen());
    combo.setOpen(false);
    CHECK(!combo.isOpen());
    combo.setPlaceholder("choose");
    CHECK(combo.getPlaceholder() == "choose");
    combo.setEnabled(false);
    CHECK(!combo.isEnabled());
    combo.setEnabled(true);
    CHECK(combo.isEnabled());

    auto& first_page = ui.addComponent<emgui::Button>("first", 0, 0, 100, 30);
    auto& second_page = ui.addComponent<emgui::Button>("second", 0, 0, 100, 30);
    auto& tabs = ui.addComponent<emgui::Tabs>(20, 250, 400, 200);
    int tab_index = -1;
    tabs.onSelectionChanged([&tab_index](emgui::Tabs&, int index) {
        tab_index = index;
    });
    CHECK(tabs.getComponentName() == "Tabs");
    CHECK(tabs.addPage("First", &first_page) == 0);
    CHECK(tabs.addPage("Second", &second_page) == 1);
    CHECK(tabs.getPageCount() == 2);
    CHECK(tabs.getPage(0).title == "First");
    CHECK(tabs.getPage(0).component == &first_page);
    CHECK(tabs.getPage(1).component == &second_page);
    tabs.setActivePage(1);
    CHECK(tabs.getActivePage() == 1);
    CHECK(tab_index == 1);
}

TEST(gui_value_components) {
    emgui::Window window("core-values", 600, 500, SDL_WINDOW_SHOWN);
    auto& ui = window.getUIComponentManager();

    auto& slider = ui.addComponent<emgui::Slider>(0.0f, 100.0f, 25.0f, 20, 20, 260, 56);
    int slider_changes = 0;
    slider.onValueChanged([&slider_changes](emgui::Slider&) { ++slider_changes; });
    CHECK(slider.getComponentName() == "Slider");
    CHECK(slider.getMinimum() == 0.0f);
    CHECK(slider.getMaximum() == 100.0f);
    CHECK(slider.getValue() == 25.0f);
    CHECK(slider.getNormalizedValue() > 0.24f);
    CHECK(slider.getNormalizedValue() < 0.26f);
    slider.setStep(5.0f);
    CHECK(slider.getStep() == 5.0f);
    slider.setValue(27.0f);
    CHECK(slider.getValue() == 25.0f);
    slider.setValue(30.0f);
    CHECK(slider.getValue() == 30.0f);
    CHECK(slider_changes == 1);
    slider.setRange(0.0f, 200.0f);
    CHECK(slider.getMaximum() == 200.0f);
    slider.setEnabled(false);
    CHECK(!slider.isEnabled());
    slider.setEnabled(true);
    CHECK(slider.isEnabled());

    auto& progress = ui.addComponent<emgui::ProgressBar>(0.0f, 100.0f, 40.0f, 20, 100);
    int progress_changes = 0;
    progress.onValueChanged([&progress_changes](emgui::ProgressBar&) {
        ++progress_changes;
    });
    CHECK(progress.getComponentName() == "ProgressBar");
    CHECK(progress.getMinimum() == 0.0f);
    CHECK(progress.getMaximum() == 100.0f);
    CHECK(progress.getValue() == 40.0f);
    CHECK(progress.getNormalizedValue() > 0.39f);
    CHECK(progress.getNormalizedValue() < 0.41f);
    progress.setRange(0.0f, 200.0f);
    progress.setValue(50.0f);
    CHECK(progress.getValue() == 50.0f);
    CHECK(progress_changes >= 1);
    progress.setIndeterminate(true);
    CHECK(progress.isIndeterminate());
    progress.setIndeterminate(false);
    CHECK(!progress.isIndeterminate());
    progress.setValueLabel("Loading");
    CHECK(progress.getValueLabel() == "Loading");
}
