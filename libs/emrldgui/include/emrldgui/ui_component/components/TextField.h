// Created by epax on 22.11.26.

#pragma once

#include <cstddef>
#include <functional>
#include <string>
#include <vector>

#include "emrldgui/Window.h"
#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/event/EventTextInput.h"
#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

namespace emgui {

    class StyleValues;

    // Single-line text input with selection, clipboard, undo/redo and CSS styling
    class TextField : public UIComponent {
    public:
        using Action = std::function<void(TextField&)>;

        TextField(
            std::string placeholder = {},
            int x = 0,
            int y = 0,
            int width = 240,
            int height = 48
        );
        ~TextField() override;

        TextField(const TextField&) = delete;
        TextField& operator=(const TextField&) = delete;
        TextField(TextField&&) = delete;
        TextField& operator=(TextField&&) = delete;

        std::string getComponentName() const override {
            return "TextField";
        }

        void update() override;
        void render(RendererOpts& renderer) override;
        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;

        void onTextChanged(Action action);
        void onEnter(Action action);
        void onFocusGained(Action action);
        void onFocusLost(Action action);

        void setFocused(bool focused) override;
        bool isFocused() const override {
            return focused_;
        }

        static TextField* getFocusedField() {
            return focusedField_;
        }

        void setText(std::string text);
        const std::string& getText() const {
            return text_;
        }

        // Clipboard + selection helpers
        bool copy();
        bool cut();
        bool paste();
        void selectAll();
        void clearSelection();
        bool hasSelection() const {
            return selectionAnchor_ != caret_;
        }
        std::string getSelectedText() const;
        std::size_t getSelectionStart() const;
        std::size_t getSelectionEnd() const;

        // Simple text undo/redo, capped to avoid unbounded memory growth
        void undo();
        void redo();
        bool canUndo() const {
            return !undoStack_.empty();
        }
        bool canRedo() const {
            return !redoStack_.empty();
        }

        void setPlaceholder(std::string placeholder) {
            placeholder_ = std::move(placeholder);
        }

        const std::string& getPlaceholder() const {
            return placeholder_;
        }

        void setSize(int width, int height) {
            width_ = width;
            height_ = height;
        }

        int getWidth() const {
            return width_;
        }

        int getHeight() const {
            return height_;
        }

        void setEnabled(bool enabled);
        bool isEnabled() const {
            return enabled_;
        }

        const SDL_Color& getTextColor() const {
            return textColor_;
        }

        const std::string& getFontFamily() const {
            return fontSpec_.family;
        }

        int getFontSize() const {
            return fontSpec_.size;
        }

    private:
        struct TextSnapshot {
            std::string text;
            std::size_t caret = 0;
        };

        void onMouseEvent(EventMouse& event);
        void onKeyboardEvent(EventKeyboard& event);
        void onTextInputEvent(EventTextInput& event);

        bool containsPoint(int x, int y) const;
        int getTextStartX(int textWidth) const;
        int measureTextAdvance(
            const RendererOpts& renderer,
            const std::string& text
        ) const;
        void drawStyledText(
            RendererOpts& renderer,
            const std::string& text,
            int x,
            int y,
            const SDL_Color& color
        ) const;
        int getCaretX(const RendererOpts& renderer, std::size_t index, int textX) const;
        std::size_t getCaretIndexFromX(const RendererOpts& renderer, int x, int textX) const;

        void setCaret(std::size_t index, bool extendSelection = false);
        bool replaceRange(std::size_t start, std::size_t end, const std::string& text);
        void replaceSelection(const std::string& text);
        void deleteSelection();
        void insertText(const std::string& text);
        void deleteBeforeCaret();
        void deleteAtCaret();
        void moveCaretToStart(bool extendSelection = false);
        void moveCaretToEnd(bool extendSelection = false);
        void moveCaretLeft(bool extendSelection = false);
        void moveCaretRight(bool extendSelection = false);
        void resetCaretBlink();
        void pushUndo();
        void notifyTextChanged();
        void runActions(const std::vector<Action>& actions);

        EventManager::ListenerId mouseListener_ = EventManager::kInvalidListener;
        EventManager::ListenerId keyboardListener_ = EventManager::kInvalidListener;
        EventManager::ListenerId textInputListener_ = EventManager::kInvalidListener;

        std::string text_;
        std::string placeholder_;
        std::size_t caret_ = 0;
        std::size_t selectionAnchor_ = 0;
        int width_ = 240;
        int height_ = 48;

        bool enabled_ = true;
        bool focused_ = false;
        bool selectingWithMouse_ = false;
        bool caretVisible_ = true;
        Uint32 caretBlinkStart_ = 0;
        static TextField* focusedField_;

        SurfaceStyle surface_;
        GeometryStyle geometry_;
        Padding padding_;

        std::vector<SDL_Color> focusGradient_;
        std::vector<SDL_Color> disabledGradient_;
        SDL_Color backgroundFocusColor_{37, 37, 64, 255};
        SDL_Color disabledBackgroundColor_{55, 55, 55, 255};

        SDL_Color borderFocusColor_{110, 168, 254, 255};
        SDL_Color disabledBorderColor_{90, 90, 90, 255};

        SDL_Color textColor_{240, 244, 255, 255};
        SDL_Color placeholderColor_{135, 140, 160, 255};
        SDL_Color disabledTextColor_{180, 180, 180, 255};
        SDL_Color caretColor_{110, 168, 254, 255};
        SDL_Color selectionColor_{70, 120, 210, 190};
        SDL_Color selectionTextColor_{255, 255, 255, 255};
        int caretWidth_ = 2;
        int caretBlinkMs_ = 500;

        TextStyle textStyle_;

        FontSpec fontSpec_{"LiberationSans", 20, "normal"};
        std::string textAlign_ = "left";
        std::size_t maxLength_ = 0;

        std::vector<TextSnapshot> undoStack_;
        std::vector<TextSnapshot> redoStack_;
        std::vector<Action> textChangedActions_;
        std::vector<Action> enterActions_;
        std::vector<Action> focusGainedActions_;
        std::vector<Action> focusLostActions_;
    };

} // emgui
