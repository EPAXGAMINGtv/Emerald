// Created by epax on 24.11.26.

#pragma once

#include "emrldgui/event/EventKeyboard.h"
#include "emrldgui/event/EventManager.h"
#include "emrldgui/event/EventMouse.h"
#include "emrldgui/ui_component/UIComponent.h"
#include "emrldgui/ui_component/UiStyle.h"

#include <cstddef>
#include <functional>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace emgui {

    class StyleValues;
    class UIComponentManager;

    // CSS-styled modal dialog. Its panel, buttons and dimmed backdrop are all
    // drawn in renderOverlay() so they cover every regular widget.
    class Dialog : public UIComponent {
    public:
        using Action = std::function<void(Dialog&)>;

        Dialog() = default;
        ~Dialog() override;

        Dialog(const Dialog&) = delete;
        Dialog& operator=(const Dialog&) = delete;
        Dialog(Dialog&&) = delete;
        Dialog& operator=(Dialog&&) = delete;

        std::string getComponentName() const override {
            return "Dialog";
        }

        void render(RendererOpts& renderer) override;
        void renderOverlay(RendererOpts& renderer) override;
        void applyStyle(const StyleValues& values) override;
        void onWindowAttached(Window& window) override;
        void collectFocusable(std::vector<UIComponent*>& out) override;

        // Registers with the window's manager (if needed), makes this dialog
        // modal and moves focus to the first button.
        void show(Window& window);
        void show();
        void close();
        bool isOpen() const {
            return open_;
        }

        void setTitle(std::string title) {
            title_ = std::move(title);
        }
        const std::string& getTitle() const {
            return title_;
        }

        void setMessage(std::string message) {
            message_ = std::move(message);
        }
        const std::string& getMessage() const {
            return message_;
        }

        void addButton(std::string label, Action action);
        std::size_t getButtonCount() const {
            return buttons_.size();
        }

        void onClose(Action action) {
            closeAction_ = std::move(action);
        }

        void setResult(int result) {
            result_ = result;
        }
        int getResult() const {
            return result_;
        }

    private:
        // Small focus proxy drawn by the dialog itself. It is deliberately not
        // registered with the manager; only collectFocusable() exposes it.
        class DialogButton : public UIComponent {
        public:
            DialogButton(std::string label, Action action)
                : label_(std::move(label)),
                  action_(std::move(action)) {
                setFocusable(true);
            }

            std::string getComponentName() const override {
                return "Dialog";
            }

            void render(RendererOpts& renderer) override {
                (void)renderer;
            }

            const std::string& getLabel() const {
                return label_;
            }

            const Action& getAction() const {
                return action_;
            }

        private:
            std::string label_;
            Action action_;
        };

        void onMouseEvent(EventMouse& event);
        void onKeyboardEvent(EventKeyboard& event);
        void focusButton(int index);
        int getFocusedButtonIndex() const;
        void activateButton(int index);
        bool isOwnButton(const UIComponent* component) const;

        static int hitTest(const std::vector<SDL_Rect>& rects, int x, int y);
        SDL_Rect getViewportRect(RendererOpts& renderer) const;
        SDL_Rect getPanelRect(RendererOpts& renderer) const;
        SDL_Rect getPanelRect(RendererOpts& renderer, const SDL_Rect& viewport) const;
        std::vector<SDL_Rect> getButtonRects(RendererOpts& renderer, const SDL_Rect& panel) const;

        EventManager::ListenerId mouseListener_ = EventManager::kInvalidListener;
        EventManager::ListenerId keyboardListener_ = EventManager::kInvalidListener;
        UIComponentManager* manager_ = nullptr;

        std::vector<std::unique_ptr<DialogButton>> buttons_;
        std::string title_;
        std::string message_;
        Action closeAction_;
        int result_ = 0;
        int hoveredIndex_ = -1;
        int pressedIndex_ = -1;
        bool open_ = false;

        SDL_Color backdropColor_{0, 0, 0, 150};
        SDL_Color titleColor_{36, 36, 36, 255};
        SDL_Color textColor_{36, 36, 36, 255};
        SDL_Color buttonBackgroundColor_{255, 255, 255, 255};
        SDL_Color buttonHoverColor_{216, 233, 251, 255};
        SDL_Color buttonPressedColor_{187, 214, 245, 255};
        SDL_Color buttonBorderColor_{192, 192, 192, 255};
        SDL_Color buttonTextColor_{36, 36, 36, 255};
        SDL_Color buttonHoverTextColor_{36, 36, 36, 255};

        int panelWidth_ = 480;
        int gap_ = 12;
        int buttonGap_ = 12;
        int buttonHeight_ = 42;
        int buttonRadius_ = 8;
        int buttonBorderWidth_ = 1;
        int buttonPadding_ = 20;
        int buttonMinWidth_ = 96;
        int maxLines_ = 8;

        Padding padding_;
        SurfaceStyle surface_;
        FontSpec fontSpec_{"LiberationSans", 16, "normal"};
        FontSpec titleFontSpec_{"LiberationSans", 21, "bold"};
        FontSpec buttonFontSpec_{"LiberationSans", 16, "normal"};
    };

    // Convenience dialog with the usual button sets and integer results.
    class MessageBox : public Dialog {
    public:
        enum class Buttons {
            Ok,
            OkCancel,
            YesNo,
            YesNoCancel
        };

        static constexpr int ResultNone = -1;
        static constexpr int ResultOk = 1;
        static constexpr int ResultCancel = 0;
        static constexpr int ResultYes = 1;
        static constexpr int ResultNo = 0;

        static constexpr int OkResult = ResultOk;
        static constexpr int CancelResult = ResultCancel;
        static constexpr int YesResult = ResultYes;
        static constexpr int NoResult = ResultNo;

        MessageBox(std::string title, std::string message, Buttons buttons = Buttons::Ok);

        // Same name as Dialog so it keeps using Dialog.css.
        std::string getComponentName() const override {
            return "Dialog";
        }
    };

} // emgui
