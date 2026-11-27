#include "TestFramework.hpp"

#include "emrldgui/Window.h"
#include "emrldgui/animation/Animation.h"
#include "emrldgui/dragdrop/DragDropManager.h"
#include "emrldgui/resource/ResourceManager.h"
#include "emrldgui/style/CssParser.h"
#include "emrldgui/style/StyleManager.h"
#include "emrldgui/style/StyleResolver.h"
#include "emrldgui/ui_component/UiStyle.h"
#include "emrldgui/ui_component/components/Button.h"
#include "emrldgui/ui_component/components/DockPanel.h"
#include "emrldgui/ui_component/components/Layout.h"
#include "emrldgui/ui_component/components/Panel.h"
#include "emrldgui/ui_component/components/ScrollArea.h"
#include "emrldgui/ui_component/components/Splitter.h"
#include "emrldgui/ui_component/components/Table.h"
#include "emrldgui/ui_component/components/TreeView.h"

#include <SDL.h>

#include <any>
#include <filesystem>
#include <string>
#include <vector>

TEST(gui_style_values_css_and_resolver) {
    emgui::StyleValues values;
    values.set("color", "#ff0000");
    values.set("width", "120px");
    values.set("visible", "true");
    values.set("opacity", "0.5");
    CHECK(values.contains("color"));
    CHECK(!values.contains("missing"));
    CHECK(values.getString("color") == "#ff0000");
    CHECK(values.getString("missing", "fallback") == "fallback");
    CHECK(values.getInt("width", 0) == 120);
    CHECK(values.getInt("missing", 7) == 7);
    CHECK(values.getBool("visible", false));
    CHECK(!values.getBool("missing", false));
    CHECK(values.getFloat("opacity", 0.0f) > 0.49f);
    CHECK(values.getFloat("opacity", 0.0f) < 0.51f);
    CHECK(values.getColor("color", SDL_Color{0, 0, 0, 0}).r == 255);
    CHECK(values.getColor("missing", SDL_Color{9, 8, 7, 6}).a == 6);

    emgui::StyleValues theme;
    theme.set("color", "#0000ff");
    theme.set("padding", "4px");
    values.merge(theme);
    CHECK(values.getString("color") == "#0000ff");
    CHECK(values.getInt("padding", 0) == 4);

    CHECK(emgui::lowerCase("AbC") == "abc");
    CHECK(emgui::isAuto("AUTO"));
    CHECK(!emgui::isAuto("120px"));
    CHECK_NEAR(emgui::easeOutCubic(0.0f), 0.0f);
    CHECK_NEAR(emgui::easeOutCubic(1.0f), 1.0f);
    CHECK(emgui::withOpacity(SDL_Color{10, 20, 30, 200}, 0.5f).a == 100);
    CHECK(emgui::blendColor(
        SDL_Color{0, 0, 0, 255}, SDL_Color{100, 100, 100, 255}, 0.5f
    ).r == 50);

    emgui::Padding padding;
    CHECK(padding.horizontal() == 0);
    CHECK(padding.vertical() == 0);
    padding.apply(values);
    CHECK(padding.left == 4);
    CHECK(padding.right == 4);
    CHECK(padding.top == 4);
    CHECK(padding.bottom == 4);

    const emgui::StyleSheet sheet = emgui::CssParser::parseString(
        "Button.primary { color: #112233; width: 42px; }\n"
        "#save:hover { color: white; }\n"
        "* { padding: 6px; }\n"
    );
    CHECK(sheet.getRules().size() == 3U);
    CHECK(sheet.getRules()[0].component == "Button");
    CHECK(sheet.getRules()[0].styleClass == "primary");
    CHECK(sheet.getRules()[0].declarations.size() == 2U);
    CHECK(sheet.getRules()[1].id == "save");
    CHECK(sheet.getRules()[1].pseudoClass == "hover");
    CHECK(sheet.getRules()[2].selector == "*");

    const emgui::StyleValues resolved =
        emgui::StyleResolver::resolve(sheet, "Button", "primary", "");
    CHECK(resolved.getString("color") == "#112233");
    CHECK(resolved.getInt("width", 0) == 42);
    CHECK(resolved.getInt("padding", 0) == 6);

    emgui::StyleManager manager;
    manager.loadComponent("Button");
    CHECK(manager.getSheet("Button") != nullptr);
    CHECK(!manager.resolveNamed("Window").getString("font-family").empty());
    manager.reload();
    CHECK(manager.getSheet("Button") != nullptr);

    emgui::StyleSheet owned;
    emgui::CssRule rule;
    rule.selector = "Panel";
    rule.component = "Panel";
    owned.addRule(rule);
    CHECK(owned.getRules().size() == 1U);
    owned.clear();
    CHECK(owned.getRules().empty());
}

TEST(gui_animation_engine) {
    emgui::Animation::cancelAll();
    CHECK_NEAR(emgui::Animation::ease(emgui::Animation::Easing::Linear, 0.5f), 0.5f);
    CHECK_NEAR(emgui::Animation::ease(emgui::Animation::Easing::EaseInQuad, 0.0f), 0.0f);
    CHECK_NEAR(emgui::Animation::ease(emgui::Animation::Easing::EaseInQuad, 1.0f), 1.0f);
    CHECK(emgui::Animation::ease(emgui::Animation::Easing::EaseOutCubic, 0.5f) > 0.5f);

    float last = 0.0f;
    int updates = 0;
    int completions = 0;
    const Uint32 start = SDL_GetTicks();
    const emgui::Animation::Handle handle = emgui::Animation::run(
        0.0f, 100.0f, 50,
        [&last, &updates](float value) noexcept {
            last = value;
            ++updates;
        },
        [&completions]() noexcept { ++completions; },
        emgui::Animation::Easing::Linear
    );
    CHECK(handle != emgui::Animation::kInvalidHandle);
    CHECK(emgui::Animation::getActiveCount() >= 1U);
    emgui::Animation::tick(start + 100U);
    CHECK(completions == 1);
    CHECK(updates >= 1);
    CHECK_NEAR(last, 100.0f);

    float tweened = 5.0f;
    const emgui::Animation::Handle tween = emgui::Animation::tween(
        tweened, 50.0f, 500, emgui::Animation::Easing::Linear
    );
    CHECK(tween != emgui::Animation::kInvalidHandle);
    emgui::Animation::cancel(tween);
    emgui::Animation::cancel(handle);
    emgui::Animation::cancelAll();
    CHECK(emgui::Animation::getActiveCount() == 0U);
}

TEST(gui_drag_and_drop) {
    emgui::Window window("dnd", 600, 400, SDL_WINDOW_SHOWN);
    auto& ui = window.getUIComponentManager();
    auto& source = ui.addComponent<emgui::Button>("source", 20, 20, 100, 40);
    auto& target = ui.addComponent<emgui::Button>("target", 200, 20, 100, 40);

    emgui::DragDropManager& drag = emgui::DragDropManager::instance();
    drag.endDrag(false);
    drag.clearPayload();
    drag.setEnabled(true);
    CHECK(drag.isEnabled());
    CHECK(!drag.isDragging());
    CHECK(drag.getDragSource() == nullptr);
    CHECK(drag.getDropTarget() == nullptr);

    int begins = 0;
    int drops = 0;
    int ends = 0;
    drag.onDragBegin([&begins](emgui::UIComponent*, const std::any&) noexcept { ++begins; });
    drag.onDrop([&drops](emgui::UIComponent*, emgui::UIComponent*, const std::any&) noexcept {
        ++drops;
    });
    drag.onDragEnd([&ends](emgui::UIComponent*, bool) noexcept { ++ends; });

    drag.beginDrag(&source, std::string("payload"), 11, 22);
    CHECK(drag.isDragging());
    CHECK(drag.getDragSource() == &source);
    CHECK(begins == 1);
    CHECK(std::any_cast<std::string>(drag.getPayload()) == "payload");
    CHECK(drag.getDragPosition().x == 11);
    CHECK(drag.getDragPosition().y == 22);

    drag.setDropTarget(&target);
    CHECK(drag.getDropTarget() == &target);
    drag.updateDrag(88, 99);
    CHECK(drag.getDragPosition().x == 88);
    CHECK(drag.getDragPosition().y == 99);

    drag.endDrag(true);
    CHECK(!drag.isDragging());
    CHECK(drops == 1);
    CHECK(ends == 1);
    drag.clearPayload();
    CHECK(!drag.getPayload().has_value());

    drag.setEnabled(false);
    CHECK(!drag.isEnabled());
    drag.setEnabled(true);
    CHECK(drag.isEnabled());
}

TEST(gui_table_and_tree_views) {
    emgui::Window window("data-views", 800, 600, SDL_WINDOW_SHOWN);
    auto& ui = window.getUIComponentManager();

    auto& table = ui.addComponent<emgui::Table>(20, 20, 400, 300);
    table.setColumns({
        {"name", "Name", 140, true},
        {"value", "Value", 120, true}
    });
    table.setRows({{"beta", "2"}, {"alpha", "1"}, {"gamma", "3"}});
    CHECK(table.getComponentName() == "Table");
    CHECK(table.getColumns().size() == 2U);
    CHECK(table.getColumns()[0].key == "name");
    CHECK(table.getRows().size() == 3U);
    CHECK(table.getRowHeight() == 30);
    table.setRowHeight(28);
    CHECK(table.getRowHeight() == 28);

    int selected_row = -1;
    std::string sorted_key;
    table.onRowSelected([&selected_row](emgui::Table&, int row) noexcept {
        selected_row = row;
    });
    table.onColumnSorted([&sorted_key](emgui::Table&, const std::string& key, bool) {
        sorted_key = key;
    });
    table.selectRow(1);
    CHECK(table.getSelectedRow() == 1);
    CHECK(selected_row == 1);
    table.sortBy("name", true);
    CHECK(table.getSortKey() == "name");
    CHECK(table.isSortAscending());
    CHECK(sorted_key == "name");
    CHECK(table.getRows()[0][0] == "alpha");
    table.sortBy("name", false);
    CHECK(!table.isSortAscending());
    CHECK(table.getRows()[0][0] == "gamma");

    auto& tree = ui.addComponent<emgui::TreeView>(450, 20, 300, 300);
    tree.setRoots({
        {"root-a", "Root A", {{"child-a", "Child A", {}, false}}, true},
        {"root-b", "Root B", {}, false}
    });
    CHECK(tree.getComponentName() == "TreeView");
    CHECK(tree.getRoots().size() == 2U);
    emgui::TreeView::Node* child = tree.findNode("child-a");
    CHECK(child != nullptr);
    CHECK(child != nullptr && child->label == "Child A");
    std::string selected_id;
    tree.onSelectionChanged([&selected_id](emgui::TreeView&, const std::string& id) {
        selected_id = id;
    });
    CHECK(tree.selectNode("child-a"));
    CHECK(tree.getSelectedId() == "child-a");
    CHECK(selected_id == "child-a");
    CHECK(tree.isExpanded("root-a"));
    tree.expandAll();
    CHECK(tree.isExpanded("root-b"));
    tree.collapseAll();
    CHECK(!tree.isExpanded("root-a"));
    tree.setExpanded("root-a", true);
    CHECK(tree.isExpanded("root-a"));
    CHECK(!tree.selectNode("missing"));
}

TEST(gui_layout_and_scroll_containers) {
    emgui::Window window("layouts", 800, 600, SDL_WINDOW_SHOWN);
    auto& ui = window.getUIComponentManager();

    auto& first = ui.addComponent<emgui::Button>("first", 0, 0, 100, 40);
    auto& second = ui.addComponent<emgui::Button>("second", 0, 0, 100, 40);
    auto& splitter = ui.addComponent<emgui::Splitter>(
        20, 20, 400, 240, emgui::Splitter::Orientation::Vertical
    );
    CHECK(splitter.getComponentName() == "Splitter");
    CHECK(splitter.getOrientation() == emgui::Splitter::Orientation::Vertical);
    CHECK(splitter.getFirst() == nullptr);
    splitter.setFirst(&first);
    splitter.setSecond(&second);
    CHECK(splitter.getFirst() == &first);
    CHECK(splitter.getSecond() == &second);
    int split_changes = 0;
    splitter.onSplitChanged([&split_changes](emgui::Splitter&) noexcept { ++split_changes; });
    splitter.setSplit(0.35f);
    CHECK(splitter.getSplit() > 0.34f);
    CHECK(splitter.getSplit() < 0.36f);
    CHECK(split_changes >= 1);
    splitter.setMinPaneSize(30);
    CHECK(splitter.getMinPaneSize() == 30);
    splitter.layout();
    CHECK(!splitter.isDragging());

    auto& center = ui.addComponent<emgui::Panel>(0, 0, 100, 60, "center");
    auto& side = ui.addComponent<emgui::Panel>(0, 0, 100, 60, "side");
    auto& dock = ui.addComponent<emgui::DockPanel>(20, 280, 400, 260);
    CHECK(dock.getComponentName() == "DockPanel");
    dock.dock(&center, emgui::DockPanel::Edge::Center);
    dock.dock(&side, emgui::DockPanel::Edge::Left, 0.3f);
    CHECK(dock.getWidgets().size() == 2U);
    dock.layout();
    CHECK(dock.isSplitterEnabled());
    dock.setSplitterEnabled(false);
    CHECK(!dock.isSplitterEnabled());
    dock.setSplitterEnabled(true);
    dock.undock(&side);
    CHECK(dock.getWidgets().size() == 1U);
    dock.clear();
    CHECK(dock.getWidgets().empty());

    auto& area = ui.addComponent<emgui::ScrollArea>(450, 20, 300, 240);
    auto& child = ui.addComponent<emgui::Button>("child", 0, 0, 220, 40);
    area.addChild(&child);
    CHECK(area.getChildren().size() == 1U);
    CHECK(area.getChildren()[0] == &child);
    area.setContentSize(220, 500);
    CHECK(area.getContentSize().x == 220);
    CHECK(area.getContentSize().y == 500);
    area.setScroll(5, 10);
    CHECK(area.getScrollX() == 0);
    CHECK(area.getScrollY() == 10);
    area.scrollBy(2, 3);
    CHECK(area.getScrollX() == 0);
    CHECK(area.getScrollY() == 13);
    area.removeChild(&child);
    CHECK(area.getChildren().empty());

    auto& layout_child = ui.addComponent<emgui::Button>("layout-child", 0, 0, 80, 30);
    auto& layout = ui.addComponent<emgui::Layout>(450, 280, 300, 200);
    CHECK(layout.getComponentName() == "Layout");
    CHECK(layout.getWidth() == 300);
    CHECK(layout.getHeight() == 200);
    layout.addChild(&layout_child);
    CHECK(layout.getChildren().size() == 1U);
    layout.setDirection(emgui::Layout::Direction::Column);
    layout.setGap(8);
    layout.setAlign(emgui::Layout::Align::Center);
    layout.setJustify(emgui::Layout::Justify::SpaceBetween);
    layout.layout();
    layout.removeChild(&layout_child);
    CHECK(layout.getChildren().empty());
    layout.clearChildren();
    CHECK(layout.getChildren().empty());
}

TEST(gui_resources_and_render_stats) {
    emgui::ResourceManager::clearCache();
    CHECK(!emgui::ResourceManager::getRoot().empty());
    CHECK(!emgui::ResourceManager::stylePath("Button").empty());
    CHECK(!emgui::ResourceManager::fontPath("LiberationSans").empty());
    CHECK(!emgui::ResourceManager::texturePath("missing.bmp").empty());

    emgui::Window window("render-stats", 500, 400, SDL_WINDOW_SHOWN);
    auto& ui = window.getUIComponentManager();
    ui.addComponent<emgui::Button>("draw me", 20, 20, 160, 40);
    emgui::RendererOpts& opts = window.getRendererOpts();
    opts.setClearColor(SDL_Color{1, 2, 3, 255});
    CHECK(opts.getClearColor().r == 1);
    CHECK(opts.getClearColor().g == 2);
    CHECK(opts.getClearColor().b == 3);

    window.updateWindow();
    window.renderUI();
    CHECK(opts.getBatchedRectCount() > 0U);
    CHECK(opts.getBatchFlushCount() > 0U);
}
