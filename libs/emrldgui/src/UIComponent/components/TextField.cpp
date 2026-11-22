// Created by epax on 22.11.26.

#include "emrldgui/ui_component/components/TextField.h"

#include "emrldgui/style/StyleValues.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <limits>
#include <utility>

namespace emgui {

    namespace {

        bool isUtf8Continuation(unsigned char byte) {
            return (byte & 0xC0U) == 0x80U;
        }

        std::size_t previousUtf8Start(const std::string& text, std::size_t position) {
            if (position == 0) {
                return 0;
            }

            std::size_t start = position - 1;

            while (start > 0 && isUtf8Continuation(
                static_cast<unsigned char>(text[start])
            )) {
                --start;
            }

            return start;
        }

        std::size_t nextUtf8End(const std::string& text, std::size_t position) {
            if (position >= text.size()) {
                return text.size();
            }

            std::size_t end = position + 1;

            while (end < text.size() && isUtf8Continuation(
                static_cast<unsigned char>(text[end])
            )) {
                ++end;
            }

            return end;
        }

        std::string lower(std::string value) {
            std::transform(
                value.begin(),
                value.end(),
                value.begin(),
                [](unsigned char c) { return static_cast<char>(std::tolower(c)); }
            );
            return value;
        }

        std::string sanitizeSingleLine(std::string value) {
            for (char& character : value) {
                if (character == '\r' || character == '\n') {
                    character = ' ';
                }
            }

            return value;
        }

    } // namespace

    TextField* TextField::focusedField_ = nullptr;

    TextField::TextField(
        std::string placeholder,
        int x,
        int y,
        int width,
        int height
    )
        : UIComponent(x, y),
          placeholder_(std::move(placeholder)),
          width_(width),
          height_(height) {
        surface_.color = {30, 30, 46, 255};
        surface_.borderColor = {85, 85, 105, 255};
        surface_.hasBorderColor = true;
        surface_.borderWidth = 2;
        surface_.radii = {8, 8, 8, 8};
        padding_.top = 10;
        padding_.right = 12;
        padding_.bottom = 10;
        padding_.left = 12;
        geometry_.width = width_;
        geometry_.height = height_;
        caretBlinkStart_ = SDL_GetTicks();
    }

    TextField::~TextField() {
        if (focusedField_ == this) {
            focusedField_ = nullptr;
            SDL_StopTextInput();
        }

        if (window_ == nullptr || window_->getWindow() == nullptr) {
            return;
        }

        if (auto* manager = EventManager::getForWindow(window_->getWindow())) {
            manager->removeListener(mouseListener_);
            manager->removeListener(keyboardListener_);
            manager->removeListener(textInputListener_);
        }
    }

    void TextField::onWindowAttached(Window& window) {
        if (mouseListener_ != EventManager::kInvalidListener) {
            return;
        }

        SDL_Window* sdlWindow = window.getWindow();

        if (sdlWindow == nullptr) {
            return;
        }

        mouseListener_ = EventManager::registerEvent(
            sdlWindow,
            EventMouse(sdlWindow),
            [this](EventMouse& event) {
                onMouseEvent(event);
            }
        );

        keyboardListener_ = EventManager::registerEvent(
            sdlWindow,
            EventKeyboard(sdlWindow),
            [this](EventKeyboard& event) {
                onKeyboardEvent(event);
            }
        );

        textInputListener_ = EventManager::registerEvent(
            sdlWindow,
            EventTextInput(sdlWindow),
            [this](EventTextInput& event) {
                onTextInputEvent(event);
            }
        );
    }

    void TextField::onTextChanged(Action action) {
        if (action) {
            textChangedActions_.push_back(std::move(action));
        }
    }

    void TextField::onEnter(Action action) {
        if (action) {
            enterActions_.push_back(std::move(action));
        }
    }

    void TextField::onFocusGained(Action action) {
        if (action) {
            focusGainedActions_.push_back(std::move(action));
        }
    }

    void TextField::onFocusLost(Action action) {
        if (action) {
            focusLostActions_.push_back(std::move(action));
        }
    }

    void TextField::applyStyle(const StyleValues& values) {
        surface_.apply(values);
        textStyle_.apply(values);
        geometry_.apply(values, width_, height_);

        width_ = geometry_.width;
        height_ = geometry_.height;

        const auto stateGradient = [&values](const char* property) {
            const auto stops = values.getColors(property);
            return stops.size() >= 2 ? stops : std::vector<SDL_Color>{};
        };

        focusGradient_ = stateGradient("background-focus-gradient");
        disabledGradient_ = stateGradient("background-disabled-gradient");

        backgroundFocusColor_ = values.getColor(
            "background-focus-color",
            backgroundFocusColor_
        );
        disabledBackgroundColor_ = values.getColor(
            "disabled-background-color",
            disabledBackgroundColor_
        );

        borderFocusColor_ = values.getColor("border-focus-color", borderFocusColor_);
        disabledBorderColor_ = values.getColor(
            "disabled-border-color",
            disabledBorderColor_
        );

        textColor_ = values.getColor("text-color", textColor_);
        placeholderColor_ = values.getColor("placeholder-color", placeholderColor_);
        disabledTextColor_ = values.getColor("disabled-text-color", disabledTextColor_);
        caretColor_ = values.getColor("caret-color", caretColor_);
        selectionColor_ = values.getColor("selection-color", selectionColor_);
        selectionTextColor_ = values.getColor(
            "selection-text-color",
            selectionTextColor_
        );

        caretWidth_ = std::max(1, values.getInt("caret-width", caretWidth_));
        caretBlinkMs_ = std::max(1, values.getInt("caret-blink-ms", caretBlinkMs_));

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);

        padding_.apply(values, 10);

        const std::string align = lower(values.getString("text-align", textAlign_));
        textAlign_ = align == "center" || align == "right" ? align : "left";

        const int maxLength = values.getInt(
            "max-length",
            static_cast<int>(maxLength_)
        );
        maxLength_ = maxLength < 0 ? 0 : static_cast<std::size_t>(maxLength);

        spdlog::debug(
            "TextField style: font={} size={} radius={} maxLength={}",
            fontSpec_.family,
            fontSpec_.size,
            surface_.maxRadius(),
            maxLength_
        );
    }

    void TextField::update() {
        if (!focused_ || !enabled_) {
            caretVisible_ = false;
            return;
        }

        const Uint32 elapsed = SDL_GetTicks() - caretBlinkStart_;
        caretVisible_ = (elapsed % static_cast<Uint32>(caretBlinkMs_)) <
                        static_cast<Uint32>(caretBlinkMs_ / 2);
    }

    void TextField::render(RendererOpts& renderer) {
        SDL_Color background = surface_.color;
        SDL_Color border = surface_.borderColor;
        SDL_Color foreground = textColor_;
        std::vector<SDL_Color> stops = surface_.gradient;

        if (focused_ && enabled_) {
            background = backgroundFocusColor_;
            border = borderFocusColor_;

            if (!focusGradient_.empty()) {
                stops = focusGradient_;
            }
        }

        if (!enabled_) {
            background = disabledBackgroundColor_;
            border = disabledBorderColor_;
            foreground = disabledTextColor_;

            if (!disabledGradient_.empty()) {
                stops = disabledGradient_;
            }
        }

        const SDL_Rect rect{x_, y_, width_, height_};

        surface_.drawShadow(renderer, rect);
        surface_.drawBackgroundStops(renderer, rect, background, stops);
        surface_.drawBorderColor(renderer, rect, border);
        surface_.drawFocus(renderer, rect, focused_ && enabled_);

        const bool usePlaceholder = text_.empty();
        const std::string& displayText = usePlaceholder ? placeholder_ : text_;
        const std::string preparedText = textStyle_.prepare(displayText);

        if (usePlaceholder && enabled_) {
            foreground = placeholderColor_;
        }

        const int textHeight = renderer.measureText(preparedText, fontSpec_).y;
        const int textWidth = measureTextAdvance(renderer, preparedText);
        const int textX = getTextStartX(textWidth);
        const int textY = std::max(
            rect.y + padding_.top,
            rect.y + padding_.top +
                std::max(0, (rect.h - padding_.vertical() - textHeight) / 2)
        );

        if (!preparedText.empty() && !hasSelection()) {
            drawStyledText(renderer, preparedText, textX, textY, foreground);
        } else if (!preparedText.empty()) {
            const std::size_t selectionStart = getSelectionStart();
            const std::size_t selectionEnd = getSelectionEnd();
            const std::string beforeText = textStyle_.prepare(
                text_.substr(0, selectionStart)
            );
            const std::string selectedText = textStyle_.prepare(
                text_.substr(selectionStart, selectionEnd - selectionStart)
            );
            const std::string afterText = textStyle_.prepare(
                text_.substr(selectionEnd)
            );
            const int beforeWidth = measureTextAdvance(renderer, beforeText);
            const int selectedWidth = std::max(
                1,
                measureTextAdvance(renderer, selectedText)
            );
            const int selectionX = textX + beforeWidth;

            renderer.fillRect(
                {
                    selectionX,
                    textY,
                    selectedWidth,
                    std::max(1, textHeight)
                },
                selectionColor_
            );

            if (!beforeText.empty()) {
                drawStyledText(renderer, beforeText, textX, textY, foreground);
            }

            if (!selectedText.empty()) {
                drawStyledText(
                    renderer,
                    selectedText,
                    selectionX,
                    textY,
                    selectionTextColor_
                );
            }

            if (!afterText.empty()) {
                drawStyledText(
                    renderer,
                    afterText,
                    selectionX + selectedWidth,
                    textY,
                    foreground
                );
            }
        }

        if (focused_ && enabled_ && caretVisible_) {
            const int caretX = getCaretX(renderer, caret_, textX);

            renderer.fillRect(
                {caretX, textY, caretWidth_, std::max(1, textHeight)},
                caretColor_
            );
        }
    }

    bool TextField::containsPoint(int x, int y) const {
        return x >= x_ && x < x_ + width_ && y >= y_ && y < y_ + height_;
    }

    int TextField::getTextStartX(int textWidth) const {
        int textX = x_ + padding_.left;

        if (textAlign_ == "right") {
            textX = x_ + width_ - padding_.right - textWidth;
        } else if (textAlign_ == "center") {
            textX = x_ + (width_ - padding_.horizontal() - textWidth) / 2 +
                    padding_.left;
        }

        return std::max(x_ + padding_.left, textX);
    }

    int TextField::measureTextAdvance(
        const RendererOpts& renderer,
        const std::string& text
    ) const {
        const int width = renderer.measureText(text, fontSpec_).x;
        const int spacing = textStyle_.letterSpacing *
                            std::max(0, static_cast<int>(text.size()) - 1);
        return width + spacing;
    }

    void TextField::drawStyledText(
        RendererOpts& renderer,
        const std::string& text,
        int x,
        int y,
        const SDL_Color& color
    ) const {
        textStyle_.draw(renderer, text, {x, y}, color, fontSpec_);
    }

    int TextField::getCaretX(
        const RendererOpts& renderer,
        std::size_t index,
        int textX
    ) const {
        const std::string prefix = textStyle_.prepare(
            text_.substr(0, std::min(index, text_.size()))
        );

        return std::min(
            textX + measureTextAdvance(renderer, prefix),
            x_ + width_ - padding_.right - caretWidth_
        );
    }

    std::size_t TextField::getCaretIndexFromX(
        const RendererOpts& renderer,
        int x,
        int textX
    ) const {
        if (text_.empty()) {
            return 0;
        }

        std::size_t best = text_.size();
        int bestDistance = std::numeric_limits<int>::max();
        std::size_t position = 0;

        while (true) {
            const int caretX = getCaretX(renderer, position, textX);
            const int distance = std::abs(caretX - x);

            if (distance < bestDistance) {
                bestDistance = distance;
                best = position;
            }

            if (caretX >= x || position == text_.size()) {
                break;
            }

            const std::size_t next = nextUtf8End(text_, position);
            position = next > position ? next : text_.size();
        }

        return best;
    }

    void TextField::resetCaretBlink() {
        caretBlinkStart_ = SDL_GetTicks();
        caretVisible_ = true;
    }

    void TextField::setCaret(std::size_t index, bool extendSelection) {
        caret_ = std::min(index, text_.size());

        if (!extendSelection) {
            selectionAnchor_ = caret_;
        }

        resetCaretBlink();
    }

    std::size_t TextField::getSelectionStart() const {
        return std::min(selectionAnchor_, caret_);
    }

    std::size_t TextField::getSelectionEnd() const {
        return std::max(selectionAnchor_, caret_);
    }

    std::string TextField::getSelectedText() const {
        if (!hasSelection()) {
            return {};
        }

        const std::size_t start = getSelectionStart();
        return text_.substr(start, getSelectionEnd() - start);
    }

    void TextField::selectAll() {
        caret_ = text_.size();
        selectionAnchor_ = 0;
        resetCaretBlink();
    }

    void TextField::clearSelection() {
        selectionAnchor_ = caret_;
    }

    void TextField::pushUndo() {
        undoStack_.push_back({text_, caret_});

        if (undoStack_.size() > 100) {
            undoStack_.erase(undoStack_.begin());
        }

        redoStack_.clear();
    }

    bool TextField::replaceRange(
        std::size_t start,
        std::size_t end,
        const std::string& text
    ) {
        start = std::min(start, text_.size());
        end = std::clamp(end, start, text_.size());

        std::string next = text_;
        next.replace(start, end - start, text);

        if (maxLength_ > 0 && next.size() > maxLength_) {
            return false;
        }

        if (next == text_) {
            caret_ = start + text.size();
            selectionAnchor_ = caret_;
            resetCaretBlink();
            return false;
        }

        pushUndo();
        text_ = std::move(next);
        caret_ = start + text.size();
        selectionAnchor_ = caret_;
        resetCaretBlink();
        notifyTextChanged();
        return true;
    }

    void TextField::replaceSelection(const std::string& text) {
        replaceRange(getSelectionStart(), getSelectionEnd(), text);
    }

    void TextField::deleteSelection() {
        if (hasSelection()) {
            replaceSelection({});
        }
    }

    bool TextField::copy() {
        if (!hasSelection()) {
            return false;
        }

        return SDL_SetClipboardText(getSelectedText().c_str()) == 0;
    }

    bool TextField::cut() {
        if (!copy()) {
            return false;
        }

        deleteSelection();
        return true;
    }

    bool TextField::paste() {
        char* clipboard = SDL_GetClipboardText();

        if (clipboard == nullptr) {
            return false;
        }

        const std::string text = sanitizeSingleLine(clipboard);
        SDL_free(clipboard);

        if (text.empty()) {
            return false;
        }

        replaceSelection(text);
        return true;
    }

    void TextField::undo() {
        if (undoStack_.empty()) {
            return;
        }

        const TextSnapshot snapshot = undoStack_.back();
        undoStack_.pop_back();
        redoStack_.push_back({text_, caret_});

        text_ = snapshot.text;
        caret_ = std::min(snapshot.caret, text_.size());
        selectionAnchor_ = caret_;
        resetCaretBlink();
        notifyTextChanged();
    }

    void TextField::redo() {
        if (redoStack_.empty()) {
            return;
        }

        const TextSnapshot snapshot = redoStack_.back();
        redoStack_.pop_back();
        undoStack_.push_back({text_, caret_});

        text_ = snapshot.text;
        caret_ = std::min(snapshot.caret, text_.size());
        selectionAnchor_ = caret_;
        resetCaretBlink();
        notifyTextChanged();
    }

    void TextField::setFocused(bool focused) {
        if (focused && !enabled_) {
            return;
        }

        if (focused) {
            if (focusedField_ != nullptr && focusedField_ != this) {
                focusedField_->setFocused(false);
            }

            if (!focused_) {
                focused_ = true;
                resetCaretBlink();
                focusedField_ = this;
                SDL_StartTextInput();
                runActions(focusGainedActions_);
                return;
            }

            focusedField_ = this;
            SDL_StartTextInput();
            return;
        }

        if (!focused_) {
            return;
        }

        focused_ = false;
        caretVisible_ = false;
        selectingWithMouse_ = false;

        if (focusedField_ == this) {
            focusedField_ = nullptr;
            SDL_StopTextInput();
        }

        runActions(focusLostActions_);
    }

    void TextField::setEnabled(bool enabled) {
        enabled_ = enabled;

        if (!enabled_) {
            setFocused(false);
        }
    }

    void TextField::setText(std::string text) {
        replaceRange(0, text_.size(), text);
    }

    void TextField::insertText(const std::string& text) {
        if (text.empty() || !enabled_) {
            return;
        }

        replaceSelection(text);
    }

    void TextField::deleteBeforeCaret() {
        if (hasSelection()) {
            deleteSelection();
            return;
        }

        if (caret_ == 0) {
            return;
        }

        const std::size_t start = previousUtf8Start(text_, caret_);
        replaceRange(start, caret_, {});
    }

    void TextField::deleteAtCaret() {
        if (hasSelection()) {
            deleteSelection();
            return;
        }

        if (caret_ >= text_.size()) {
            return;
        }

        const std::size_t end = nextUtf8End(text_, caret_);
        replaceRange(caret_, end, {});
    }

    void TextField::moveCaretToStart(bool extendSelection) {
        setCaret(0, extendSelection);
    }

    void TextField::moveCaretToEnd(bool extendSelection) {
        setCaret(text_.size(), extendSelection);
    }

    void TextField::moveCaretLeft(bool extendSelection) {
        if (caret_ == 0) {
            return;
        }

        setCaret(previousUtf8Start(text_, caret_), extendSelection);
    }

    void TextField::moveCaretRight(bool extendSelection) {
        if (caret_ >= text_.size()) {
            return;
        }

        setCaret(nextUtf8End(text_, caret_), extendSelection);
    }

    void TextField::notifyTextChanged() {
        runActions(textChangedActions_);
    }

    void TextField::onMouseEvent(EventMouse& event) {
        if (!enabled_) {
            return;
        }

        const bool press = event.isPressed();
        const bool release = event.isReleased();

        if (event.getButton() != SDL_BUTTON_LEFT && (press || release)) {
            return;
        }

        if (press) {
            const bool inside = containsPoint(event.getX(), event.getY());
            setFocused(inside);

            if (!inside) {
                return;
            }

            const RendererOpts& renderer = window_->getRendererOpts();
            const SDL_Point size = renderer.measureText(text_, fontSpec_);
            const int textX = getTextStartX(size.x);
            const std::size_t index = getCaretIndexFromX(renderer, event.getX(), textX);
            const bool extend = (SDL_GetModState() & KMOD_SHIFT) != 0;

            setCaret(index, extend);
            selectingWithMouse_ = true;
            return;
        }

        if (event.isMoved() && selectingWithMouse_) {
            const RendererOpts& renderer = window_->getRendererOpts();
            const SDL_Point size = renderer.measureText(text_, fontSpec_);
            const int textX = getTextStartX(size.x);
            const std::size_t index = getCaretIndexFromX(renderer, event.getX(), textX);
            caret_ = index;
            resetCaretBlink();
            return;
        }

        if (release) {
            selectingWithMouse_ = false;
        }
    }

    void TextField::onKeyboardEvent(EventKeyboard& event) {
        if (!focused_ || !enabled_ || !event.isPressed()) {
            return;
        }

        const SDL_Keymod modifiers = event.getModifiers();
        const bool command = (modifiers & (KMOD_CTRL | KMOD_GUI)) != 0;
        const bool shift = (modifiers & KMOD_SHIFT) != 0;
        const SDL_Keycode key = event.getKey();

        if (command) {
            switch (key) {
                case SDLK_a:
                    selectAll();
                    return;
                case SDLK_c:
                    copy();
                    return;
                case SDLK_x:
                    cut();
                    return;
                case SDLK_v:
                    paste();
                    return;
                case SDLK_z:
                    if (shift) {
                        redo();
                    } else {
                        undo();
                    }
                    return;
                case SDLK_y:
                    redo();
                    return;
                default:
                    break;
            }
        }

        switch (key) {
            case SDLK_BACKSPACE:
                deleteBeforeCaret();
                break;
            case SDLK_DELETE:
                deleteAtCaret();
                break;
            case SDLK_LEFT:
                moveCaretLeft(shift);
                break;
            case SDLK_RIGHT:
                moveCaretRight(shift);
                break;
            case SDLK_HOME:
                moveCaretToStart(shift);
                break;
            case SDLK_END:
                moveCaretToEnd(shift);
                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                runActions(enterActions_);
                break;
            case SDLK_ESCAPE:
                setFocused(false);
                break;
            default:
                break;
        }
    }

    void TextField::onTextInputEvent(EventTextInput& event) {
        if (!focused_ || !enabled_) {
            return;
        }

        insertText(event.getText());
    }

    void TextField::runActions(const std::vector<Action>& actions) {
        const auto snapshot = actions;

        for (const Action& action : snapshot) {
            try {
                action(*this);
            } catch (const std::exception& error) {
                spdlog::error("TextField action failed: {}", error.what());
            }
        }
    }

} // emgui
