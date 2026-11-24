// Created by epax on 24.11.26.

#pragma once

#include <algorithm>
#include <cstddef>
#include <functional>
#include <string>
#include <utility>
#include <vector>

#include "emrldgui/Window.h"
#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/event/EventTextInput.h"
#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

namespace emgui {

    class StyleValues;

    // Multiline text editor with selection, clipboard, undo/redo, word wrap,
    // optional line-number gutter and CSS driven scroll indicators
    class TextArea : public UIComponent {
    public:
        using Action = std::function<void(TextArea&)>;

        TextArea(
            std::string placeholder = {},
            int x = 0,
            int y = 0,
            int width = 300,
            int height = 160
        );
        ~TextArea() override;

        // Listeners capture this, so copies and moves would dangle
        TextArea(const TextArea&) = delete;
        TextArea& operator=(const TextArea&) = delete;
        TextArea(TextArea&&) = delete;
        TextArea& operator=(TextArea&&) = delete;

        std::string getComponentName() const override {
            return "TextArea";
        }

        void update() override;
        void render(RendererOpts& renderer) override;
        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;

        SDL_Point getLayoutSize() const override {
            return {width_, height_};
        }

        void setFocused(bool focused) override;
        bool isFocused() const override {
            return focused_;
        }

        static TextArea* getFocusedArea() {
            return focusedArea_;
        }

        void onTextChanged(Action action);
        void onEnter(Action action);
        void onFocusGained(Action action);
        void onFocusLost(Action action);

        void setText(std::string text);
        const std::string& getText() const {
            return text_;
        }

        void setPlaceholder(std::string placeholder) {
            placeholder_ = std::move(placeholder);
        }

        const std::string& getPlaceholder() const {
            return placeholder_;
        }

        void setSize(int width, int height) {
            width_ = std::max(0, width);
            height_ = std::max(0, height);
        }

        void setEnabled(bool enabled);
        bool isEnabled() const {
            return enabled_;
        }

        void setReadOnly(bool readOnly) {
            readOnly_ = readOnly;
        }

        bool isReadOnly() const {
            return readOnly_;
        }

        std::size_t getLineCount() const;
        std::size_t getCaretLine() const;
        std::size_t getCaretColumn() const;

    private:
        // One entry per displayed row; wrapped logical lines span several rows
        struct VisualLine {
            std::size_t logicalLine = 0;
            std::size_t start = 0;
            std::size_t end = 0;
        };

        struct TextSnapshot {
            std::string text;
            std::size_t caret = 0;
            std::size_t anchor = 0;
        };

        void onMouseEvent(EventMouse& event);
        void onKeyboardEvent(EventKeyboard& event);
        void onTextInputEvent(EventTextInput& event);

        bool containsPoint(int x, int y) const;
        SDL_Rect getInnerRect() const;
        int getGutterWidth() const;
        int getTextX() const;
        int getViewportWidth(bool reserveScrollbar = true) const;
        int getMaxScrollX() const;
        int getMaxScrollY() const;
        void clampScroll();

        std::size_t lineCount() const;
        std::size_t getLineStart(std::size_t line) const;
        std::size_t getLineEnd(std::size_t line) const;
        std::size_t lineForOffset(std::size_t offset) const;

        std::pair<std::vector<VisualLine>, int> buildRows(
            RendererOpts& renderer,
            int availableWidth
        ) const;
        void rebuildRows(RendererOpts& renderer);
        std::size_t findCaretRow() const;
        std::size_t getOffsetFromX(
            RendererOpts& renderer,
            const VisualLine& row,
            int x
        ) const;
        std::size_t getOffsetFromPoint(RendererOpts& renderer, int x, int y) const;
        int measureRange(
            const RendererOpts& renderer,
            std::size_t start,
            std::size_t end
        ) const;
        void drawSegment(
            RendererOpts& renderer,
            std::size_t start,
            std::size_t end,
            int x,
            int y,
            const SDL_Color& color
        ) const;

        void ensureCaretVisible(RendererOpts& renderer);
        void moveCaretVertically(RendererOpts& renderer, int rows, bool extend);
        void moveCaretHomeEnd(RendererOpts& renderer, bool end, bool extend);

        void setCaret(std::size_t offset, bool extendSelection = false);
        bool replaceRange(
            std::size_t start,
            std::size_t end,
            const std::string& value
        );
        void replaceSelection(const std::string& value);
        void deleteSelection();
        void insertText(const std::string& value);
        void deleteBeforeCaret();
        void deleteAtCaret();
        bool copy();
        bool cut();
        bool paste();
        void undo();
        void redo();
        void pushUndo();
        void selectAll();
        void resetCaretBlink();
        void notifyTextChanged();
        void runActions(const std::vector<Action>& actions);

        EventManager::ListenerId mouseListener_ = EventManager::kInvalidListener;
        EventManager::ListenerId keyboardListener_ = EventManager::kInvalidListener;
        EventManager::ListenerId textInputListener_ = EventManager::kInvalidListener;

        std::string text_;
        std::string placeholder_;
        std::size_t caret_ = 0;
        std::size_t selectionAnchor_ = 0;
        int width_ = 300;
        int height_ = 160;

        bool enabled_ = true;
        bool readOnly_ = false;
        bool focused_ = false;
        bool selectingWithMouse_ = false;
        bool caretVisible_ = true;
        bool wordWrap_ = true;
        bool showLineNumbers_ = false;
        Uint32 caretBlinkStart_ = 0;
        static TextArea* focusedArea_;

        int scrollX_ = 0;
        int scrollY_ = 0;
        int viewportWidth_ = 1;
        int viewportHeight_ = 1;
        int contentWidthPx_ = 0;
        std::vector<VisualLine> visualLines_;

        SurfaceStyle surface_;
        Padding padding_;
        TextStyle textStyle_;
        FontSpec fontSpec_{"LiberationSans", 16, "normal"};

        std::vector<SDL_Color> focusGradient_;
        SDL_Color backgroundFocusColor_{255, 255, 255, 255};
        SDL_Color disabledBackgroundColor_{246, 245, 244, 255};
        SDL_Color borderFocusColor_{53, 132, 228, 255};
        SDL_Color disabledBorderColor_{223, 223, 223, 255};

        SDL_Color textColor_{36, 36, 36, 255};
        SDL_Color placeholderColor_{98, 105, 114, 255};
        SDL_Color disabledTextColor_{98, 105, 114, 255};
        SDL_Color caretColor_{53, 132, 228, 255};
        SDL_Color selectionColor_{53, 132, 228, 90};
        SDL_Color selectionTextColor_{255, 255, 255, 255};
        SDL_Color gutterColor_{98, 105, 114, 255};
        SDL_Color scrollbarColor_{192, 192, 192, 255};
        SDL_Color focusRingColor_{53, 132, 228, 255};

        int focusRingWidth_ = 2;
        int caretWidth_ = 2;
        int caretBlinkMs_ = 500;
        int lineHeight_ = 20;
        int tabSize_ = 4;
        int gutterWidth_ = 40;

        std::vector<TextSnapshot> undoStack_;
        std::vector<TextSnapshot> redoStack_;
        std::vector<Action> textChangedActions_;
        std::vector<Action> enterActions_;
        std::vector<Action> focusGainedActions_;
        std::vector<Action> focusLostActions_;
    };

} // emgui
