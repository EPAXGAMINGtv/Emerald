// Created by epax on 24.11.26.

#include "emrldgui/ui_component/components/Dialog.h"

#include "emrldgui/Window.h"
#include "emrldgui/style/StyleValues.h"
#include "emrldgui/ui_component/UIComponentManager.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <exception>
#include <string>
#include <utility>
#include <vector>

namespace emgui {

    namespace {

        std::vector<std::string> wrapText(
            RendererOpts& renderer,
            const std::string& text,
            int maxWidth,
            const FontSpec& fontSpec,
            int maxLines
        ) {
            std::vector<std::string> lines;

            if (text.empty()) {
                return lines;
            }

            const int width = std::max(1, maxWidth);
            const int limit = std::max(1, maxLines);

            const auto fits = [&renderer, &fontSpec, width](const std::string& value) {
                return renderer.measureText(value, fontSpec).x <= width;
            };

            std::size_t paragraphStart = 0;
            while (true) {
                const std::string::size_type newline = text.find('\n', paragraphStart);
                const std::string paragraph = text.substr(
                    paragraphStart,
                    newline == std::string::npos ? std::string::npos : newline - paragraphStart
                );

                std::string line;
                std::size_t position = 0;

                while (position < paragraph.size()) {
                    const std::size_t space = paragraph.find_first_of(" \t", position);
                    const std::string word = paragraph.substr(
                        position,
                        space == std::string::npos ? std::string::npos : space - position
                    );
                    const std::string candidate = line.empty() ? word : line + " " + word;

                    if (fits(candidate)) {
                        line = candidate;
                    } else {
                        if (!line.empty()) {
                            lines.push_back(line);
                            line.clear();
                        }

                        if (fits(word)) {
                            line = word;
                        } else {
                            // A single word wider than the panel is split per character.
                            std::string chunk;
                            for (const char character : word) {
                                const std::string next = chunk + character;
                                if (!chunk.empty() && !fits(next)) {
                                    lines.push_back(chunk);
                                    chunk.clear();
                                }
                                chunk += character;
                            }
                            line = chunk;
                        }
                    }

                    if (space == std::string::npos) {
                        break;
                    }

                    position = paragraph.find_first_not_of(" \t", space);
                    if (position == std::string::npos) {
                        break;
                    }
                }

                lines.push_back(line);

                if (newline == std::string::npos) {
                    break;
                }
                paragraphStart = newline + 1;
            }

            if (static_cast<int>(lines.size()) > limit) {
                lines.resize(static_cast<std::size_t>(limit));

                std::string& last = lines.back();
                while (!last.empty() && !fits(last + "...")) {
                    last.pop_back();
                }
                last += "...";
            }

            return lines;
        }

    } // namespace

    Dialog::~Dialog() {
        // During shutdown the UI manager may already be gone. Only the event
        // manager is touched here, through a guarded window pointer.
        if (window_ == nullptr || window_->getWindow() == nullptr) {
            return;
        }

        if (auto* events = EventManager::getForWindow(window_->getWindow())) {
            if (mouseListener_ != EventManager::kInvalidListener) {
                events->removeListener(mouseListener_);
            }
            if (keyboardListener_ != EventManager::kInvalidListener) {
                events->removeListener(keyboardListener_);
            }
        }
    }

    void Dialog::onWindowAttached(Window& window) {
        SDL_Window* sdlWindow = window.getWindow();

        if (sdlWindow == nullptr) {
            return;
        }

        if (mouseListener_ == EventManager::kInvalidListener) {
            mouseListener_ = EventManager::registerEvent(
                sdlWindow,
                EventMouse(sdlWindow),
                [this](EventMouse& event) {
                    onMouseEvent(event);
                }
            );
        }

        if (keyboardListener_ == EventManager::kInvalidListener) {
            keyboardListener_ = EventManager::registerEvent(
                sdlWindow,
                EventKeyboard(sdlWindow),
                [this](EventKeyboard& event) {
                    onKeyboardEvent(event);
                }
            );
        }
    }

    void Dialog::applyStyle(const StyleValues& values) {
        surface_.apply(values);

        backdropColor_ = values.getColor("backdrop-color", backdropColor_);
        titleColor_ = values.getColor("title-color", titleColor_);
        textColor_ = values.getColor("text-color", textColor_);
        buttonBackgroundColor_ = values.getColor(
            "button-background-color",
            buttonBackgroundColor_
        );
        buttonHoverColor_ = values.getColor("button-hover-color", buttonHoverColor_);
        buttonPressedColor_ = values.getColor("button-pressed-color", buttonPressedColor_);
        buttonBorderColor_ = values.getColor("button-border-color", buttonBorderColor_);
        buttonTextColor_ = values.getColor("button-text-color", buttonTextColor_);
        buttonHoverTextColor_ = values.getColor(
            "button-hover-text-color",
            buttonHoverTextColor_
        );

        panelWidth_ = std::max(120, values.getInt("width", panelWidth_));
        gap_ = std::max(0, values.getInt("gap", gap_));
        buttonGap_ = std::max(0, values.getInt("button-gap", buttonGap_));
        buttonHeight_ = std::max(16, values.getInt("button-height", buttonHeight_));
        buttonRadius_ = std::max(0, values.getInt("button-radius", buttonRadius_));
        buttonBorderWidth_ = std::max(
            0,
            values.getInt("button-border-width", buttonBorderWidth_)
        );
        buttonPadding_ = std::max(0, values.getInt("button-padding", buttonPadding_));
        buttonMinWidth_ = std::max(32, values.getInt("button-min-width", buttonMinWidth_));
        maxLines_ = std::max(1, values.getInt("max-lines", maxLines_));

        padding_.apply(values, 20);

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);

        titleFontSpec_.family = fontSpec_.family;
        titleFontSpec_.style = fontSpec_.style;
        titleFontSpec_.size = std::max(1, values.getInt("title-font-size", titleFontSpec_.size));

        buttonFontSpec_.family = fontSpec_.family;
        buttonFontSpec_.style = fontSpec_.style;
        buttonFontSpec_.size = std::max(
            1,
            values.getInt("button-font-size", buttonFontSpec_.size)
        );
    }

    void Dialog::addButton(std::string label, Action action) {
        auto button = std::make_unique<DialogButton>(std::move(label), std::move(action));
        buttons_.push_back(std::move(button));

        // Keep the manager's focus pointer valid and predictable while open.
        if (open_) {
            focusButton(0);
        }
    }

    void Dialog::show(Window& window) {
        if (window.getWindow() == nullptr) {
            return;
        }

        UIComponentManager& manager = window.getUIComponentManager();
        if (!manager.contains(this)) {
            manager.addComponent(this);
        }

        manager_ = &manager;
        open_ = true;
        hoveredIndex_ = -1;
        pressedIndex_ = -1;
        setVisible(true);

        for (const auto& button : buttons_) {
            button->setHovered(false);
            button->setActive(false);
        }

        manager_->setModalComponent(this);
        focusButton(0);
    }

    void Dialog::show() {
        if (open_ || window_ == nullptr || window_->getWindow() == nullptr) {
            return;
        }

        show(*window_);
    }

    void Dialog::close() {
        if (!open_) {
            return;
        }

        open_ = false;
        hoveredIndex_ = -1;
        pressedIndex_ = -1;

        for (const auto& button : buttons_) {
            button->setHovered(false);
            button->setActive(false);
            button->setFocused(false);
        }

        if (manager_ != nullptr) {
            if (manager_->getModalComponent() == this) {
                manager_->setModalComponent(nullptr);
            }

            if (isOwnButton(manager_->getFocusedComponent())) {
                manager_->setFocusedComponent(nullptr);
                // Dialog buttons never type text, so a lingering text input
                // session belongs to a widget hidden behind the modal.
                SDL_StopTextInput();
            }

            manager_ = nullptr;
        }

        if (closeAction_) {
            try {
                closeAction_(*this);
            } catch (const std::exception& error) {
                spdlog::error("Dialog close action failed: {}", error.what());
            }
        }
    }

    bool Dialog::isOwnButton(const UIComponent* component) const {
        if (component == nullptr) {
            return false;
        }

        for (const auto& button : buttons_) {
            if (button.get() == component) {
                return true;
            }
        }

        return false;
    }

    void Dialog::focusButton(int index) {
        const bool valid = open_ && index >= 0 &&
                           static_cast<std::size_t>(index) < buttons_.size();

        for (std::size_t position = 0; position < buttons_.size(); ++position) {
            buttons_[position]->setFocused(valid && position == static_cast<std::size_t>(index));
        }

        if (manager_ != nullptr) {
            manager_->setFocusedComponent(
                valid ? buttons_[static_cast<std::size_t>(index)].get() : nullptr
            );
        }
    }

    int Dialog::getFocusedButtonIndex() const {
        if (manager_ != nullptr) {
            const UIComponent* focused = manager_->getFocusedComponent();

            for (std::size_t position = 0; position < buttons_.size(); ++position) {
                if (buttons_[position].get() == focused) {
                    return static_cast<int>(position);
                }
            }
        }

        for (std::size_t position = 0; position < buttons_.size(); ++position) {
            if (buttons_[position]->isFocused()) {
                return static_cast<int>(position);
            }
        }

        return -1;
    }

    void Dialog::activateButton(int index) {
        if (!open_ || index < 0 || static_cast<std::size_t>(index) >= buttons_.size()) {
            return;
        }

        // Copy the action so it may safely add or remove buttons.
        const Action action = buttons_[static_cast<std::size_t>(index)]->getAction();

        if (action) {
            try {
                action(*this);
            } catch (const std::exception& error) {
                spdlog::error("Dialog button action failed: {}", error.what());
            }
        }

        close();
    }

    void Dialog::collectFocusable(std::vector<UIComponent*>& out) {
        if (!open_) {
            return;
        }

        for (const auto& button : buttons_) {
            if (button->isFocusable() && !button->isDisabled()) {
                out.push_back(button.get());
            }
        }
    }

    SDL_Rect Dialog::getViewportRect(RendererOpts& renderer) const {
        int viewWidth = 0;
        int viewHeight = 0;
        SDL_Renderer* sdlRenderer = renderer.getRenderer();

        if (sdlRenderer != nullptr) {
            SDL_RenderGetLogicalSize(sdlRenderer, &viewWidth, &viewHeight);
        }

        if (viewWidth <= 0 || viewHeight <= 0) {
            viewWidth = 0;
            viewHeight = 0;

            if (window_ != nullptr) {
                viewWidth = window_->getWidth();
                viewHeight = window_->getHeight();
            }

            if ((viewWidth <= 0 || viewHeight <= 0) && sdlRenderer != nullptr) {
                SDL_GetRendererOutputSize(sdlRenderer, &viewWidth, &viewHeight);
            }
        }

        return {0, 0, std::max(0, viewWidth), std::max(0, viewHeight)};
    }

    SDL_Rect Dialog::getPanelRect(RendererOpts& renderer) const {
        return getPanelRect(renderer, getViewportRect(renderer));
    }

    SDL_Rect Dialog::getPanelRect(RendererOpts& renderer, const SDL_Rect& viewport) const {
        const int maxWidth = viewport.w > 0
            ? std::max(120, viewport.w - 40)
            : panelWidth_;
        const int panelWidth = std::min(std::max(120, panelWidth_), maxWidth);
        const int textWidth = std::max(
            1,
            panelWidth - padding_.horizontal() - 2 * surface_.borderWidth
        );
        const std::vector<std::string> lines = wrapText(
            renderer, message_, textWidth, fontSpec_, maxLines_
        );

        const int titleHeight = title_.empty() ? 0 : std::max(1, titleFontSpec_.size + 6);
        const int lineHeight = std::max(1, fontSpec_.size + 4);
        const int messageHeight = static_cast<int>(lines.size()) * lineHeight;
        const int buttonHeight = buttons_.empty() ? 0 : std::max(16, buttonHeight_);

        int height = padding_.vertical() + 2 * surface_.borderWidth + titleHeight;
        if (!title_.empty()) {
            height += gap_;
        }
        height += messageHeight;
        if (!buttons_.empty()) {
            height += gap_ + buttonHeight;
        }
        height = std::max(40, height);

        SDL_Rect panel{
            (viewport.w - panelWidth) / 2,
            (viewport.h - height) / 2,
            panelWidth,
            height
        };
        panel.x = std::max(0, panel.x);
        panel.y = std::max(0, panel.y);

        return panel;
    }

    std::vector<SDL_Rect> Dialog::getButtonRects(
        RendererOpts& renderer,
        const SDL_Rect& panel
    ) const {
        std::vector<SDL_Rect> rects;

        if (buttons_.empty()) {
            return rects;
        }

        const int count = static_cast<int>(buttons_.size());
        const int available = std::max(
            1,
            panel.w - padding_.horizontal() - 2 * surface_.borderWidth
        );
        const int gap = std::max(0, buttonGap_);

        std::vector<int> widths;
        widths.reserve(buttons_.size());

        int totalWidth = 0;
        for (const auto& button : buttons_) {
            const int measured = renderer.measureText(button->getLabel(), buttonFontSpec_).x;
            const int width = std::max(buttonMinWidth_, measured + 2 * buttonPadding_);
            widths.push_back(width);
            totalWidth += width;
        }

        const int gapTotal = gap * (count - 1);
        if (totalWidth + gapTotal > available) {
            const int each = std::max(40, (available - gapTotal) / count);
            widths.assign(buttons_.size(), each);
            totalWidth = each * count;
        }

        int cursorX = panel.x + (panel.w - totalWidth - gapTotal) / 2;
        cursorX = std::max(panel.x + surface_.borderWidth, cursorX);

        const int cursorY = std::max(
            panel.y + surface_.borderWidth,
            panel.y + panel.h - surface_.borderWidth - padding_.bottom - buttonHeight_
        );

        for (std::size_t index = 0; index < buttons_.size(); ++index) {
            rects.push_back({cursorX, cursorY, widths[index], buttonHeight_});
            cursorX += widths[index] + gap;
        }

        return rects;
    }

    int Dialog::hitTest(const std::vector<SDL_Rect>& rects, int x, int y) {
        for (std::size_t index = 0; index < rects.size(); ++index) {
            const SDL_Rect& rect = rects[index];

            if (x >= rect.x && x < rect.x + rect.w &&
                y >= rect.y && y < rect.y + rect.h) {
                return static_cast<int>(index);
            }
        }

        return -1;
    }

    void Dialog::render(RendererOpts& renderer) {
        (void)renderer;
    }

    void Dialog::renderOverlay(RendererOpts& renderer) {
        if (!open_) {
            return;
        }

        const SDL_Rect viewport = getViewportRect(renderer);
        if (viewport.w <= 0 || viewport.h <= 0) {
            return;
        }

        renderer.fillRect(viewport, backdropColor_);

        const SDL_Rect panel = getPanelRect(renderer, viewport);
        if (panel.w <= 0 || panel.h <= 0) {
            return;
        }

        const SDL_Color fallbackBackground{255, 255, 255, 255};
        const SDL_Color fallbackBorder{223, 223, 223, 255};

        surface_.drawShadow(renderer, panel);
        surface_.drawBackground(renderer, panel, fallbackBackground);
        surface_.drawBorder(renderer, panel, fallbackBorder);

        const int textX = panel.x + surface_.borderWidth + padding_.left;
        int cursorY = panel.y + surface_.borderWidth + padding_.top;

        if (!title_.empty()) {
            renderer.drawText(title_, {textX, cursorY}, titleColor_, titleFontSpec_);
            cursorY += titleFontSpec_.size + 6 + gap_;
        }

        const int textWidth = std::max(
            1,
            panel.w - padding_.horizontal() - 2 * surface_.borderWidth
        );
        const std::vector<std::string> lines = wrapText(
            renderer, message_, textWidth, fontSpec_, maxLines_
        );
        const int lineHeight = std::max(1, fontSpec_.size + 4);
        const SDL_Color foreground = withOpacity(textColor_, surface_.opacity);

        for (const std::string& line : lines) {
            renderer.drawText(line, {textX, cursorY}, foreground, fontSpec_);
            cursorY += lineHeight;
        }

        const std::vector<SDL_Rect> buttonRects = getButtonRects(renderer, panel);
        const SDL_Color buttonText = withOpacity(buttonTextColor_, surface_.opacity);
        const SDL_Color hoverText = withOpacity(buttonHoverTextColor_, surface_.opacity);

        for (std::size_t index = 0; index < buttons_.size() && index < buttonRects.size(); ++index) {
            const SDL_Rect& rect = buttonRects[index];
            const DialogButton& button = *buttons_[index];
            const bool hovered = static_cast<int>(index) == hoveredIndex_;
            const bool pressed = static_cast<int>(index) == pressedIndex_;

            SDL_Color background = buttonBackgroundColor_;
            if (hovered) {
                background = buttonHoverColor_;
            }
            if (pressed) {
                background = buttonPressedColor_;
            }

            renderer.fillRoundedRect(
                rect,
                buttonRadius_,
                withOpacity(background, surface_.opacity)
            );

            for (int inset = 0; inset < buttonBorderWidth_; ++inset) {
                const SDL_Rect border{
                    rect.x + inset,
                    rect.y + inset,
                    rect.w - 2 * inset,
                    rect.h - 2 * inset
                };

                if (border.w <= 0 || border.h <= 0) {
                    break;
                }

                renderer.drawRoundedRect(
                    border,
                    std::max(0, buttonRadius_ - inset),
                    withOpacity(buttonBorderColor_, surface_.opacity)
                );
            }

            const SDL_Point measured = renderer.measureText(button.getLabel(), buttonFontSpec_);
            const SDL_Point labelPosition{
                rect.x + std::max(0, (rect.w - measured.x) / 2),
                rect.y + std::max(0, (rect.h - measured.y) / 2)
            };
            renderer.drawText(
                button.getLabel(),
                labelPosition,
                hovered ? hoverText : buttonText,
                buttonFontSpec_
            );

            surface_.focusRing.draw(renderer, rect, buttonRadius_, button.isFocused());
        }
    }

    void Dialog::onMouseEvent(EventMouse& event) {
        if (!open_ || window_ == nullptr || window_->getWindow() == nullptr) {
            return;
        }

        // While the dialog is open the modal swallows mouse input from widgets
        // that registered their listeners earlier.
        event.handled = true;

        RendererOpts& renderer = window_->getRendererOpts();
        const SDL_Rect panel = getPanelRect(renderer);
        const std::vector<SDL_Rect> buttonRects = getButtonRects(renderer, panel);
        const int mouseX = event.getX();
        const int mouseY = event.getY();
        const int hovered = hitTest(buttonRects, mouseX, mouseY);

        if (hovered != hoveredIndex_) {
            hoveredIndex_ = hovered;

            for (std::size_t index = 0; index < buttons_.size(); ++index) {
                buttons_[index]->setHovered(static_cast<int>(index) == hoveredIndex_);
            }
        }

        if (event.getButton() != SDL_BUTTON_LEFT) {
            return;
        }

        if (event.isPressed()) {
            pressedIndex_ = hoveredIndex_;

            for (std::size_t index = 0; index < buttons_.size(); ++index) {
                buttons_[index]->setActive(static_cast<int>(index) == pressedIndex_);
            }
            return;
        }

        if (event.isReleased() && pressedIndex_ >= 0) {
            const int activated = pressedIndex_ == hoveredIndex_ ? pressedIndex_ : -1;
            pressedIndex_ = -1;

            for (const auto& button : buttons_) {
                button->setActive(false);
            }

            if (activated >= 0) {
                activateButton(activated);
            }
        }
    }

    void Dialog::onKeyboardEvent(EventKeyboard& event) {
        if (!open_ || !event.isPressed()) {
            return;
        }

        const SDL_Keycode key = event.getKey();

        if (key == SDLK_ESCAPE) {
            event.handled = true;
            close();
            return;
        }

        if (key == SDLK_RETURN || key == SDLK_KP_ENTER || key == SDLK_SPACE) {
            event.handled = true;

            int index = getFocusedButtonIndex();
            if (index < 0 && !buttons_.empty()) {
                index = 0;
            }
            activateButton(index);
            return;
        }

        if (key != SDLK_TAB || buttons_.empty()) {
            return;
        }

        event.handled = true;

        const bool backwards = (static_cast<int>(event.getModifiers()) & KMOD_SHIFT) != 0;
        const int count = static_cast<int>(buttons_.size());
        const int current = getFocusedButtonIndex();
        int next = 0;

        if (backwards) {
            next = current <= 0 ? count - 1 : current - 1;
        } else if (current >= 0) {
            next = (current + 1) % count;
        }

        focusButton(next);
    }

    MessageBox::MessageBox(std::string title, std::string message, Buttons buttons)
        : Dialog() {
        setTitle(std::move(title));
        setMessage(std::move(message));
        setResult(ResultNone);

        switch (buttons) {
            case Buttons::Ok:
                addButton("OK", [](Dialog& dialog) noexcept {
                    dialog.setResult(ResultOk);
                });
                break;

            case Buttons::OkCancel:
                addButton("OK", [](Dialog& dialog) noexcept {
                    dialog.setResult(ResultOk);
                });
                addButton("Cancel", [](Dialog& dialog) noexcept {
                    dialog.setResult(ResultCancel);
                });
                break;

            case Buttons::YesNo:
                addButton("Yes", [](Dialog& dialog) noexcept {
                    dialog.setResult(ResultYes);
                });
                addButton("No", [](Dialog& dialog) noexcept {
                    dialog.setResult(ResultNo);
                });
                break;

            case Buttons::YesNoCancel:
                addButton("Yes", [](Dialog& dialog) noexcept {
                    dialog.setResult(ResultYes);
                });
                addButton("No", [](Dialog& dialog) noexcept {
                    dialog.setResult(ResultNo);
                });
                addButton("Cancel", [](Dialog& dialog) noexcept {
                    dialog.setResult(ResultCancel);
                });
                break;
            default:
                break;
        }

        onClose([](Dialog& dialog) noexcept {
            if (dialog.getResult() == ResultNone) {
                dialog.setResult(ResultCancel);
            }
        });
    }

} // emgui
