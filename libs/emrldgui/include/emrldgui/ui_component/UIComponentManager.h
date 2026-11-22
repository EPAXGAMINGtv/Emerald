//
// Created by epax on 22.11.26.
//

#pragma once

#include <cstddef>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

#include "UIComponent.h"
#include "emrldgui/style/StyleManager.h"

namespace emgui {

    class UIComponentManager {
    private:
        //owned components are destroyed with the manager, external ones are only referenced ->
        struct ComponentEntry {
            std::unique_ptr<UIComponent> owned;
            UIComponent* external = nullptr;

            UIComponent* get() const {
                return owned != nullptr ? owned.get() : external;
            }
        };

    public:
        //manager owns the component, usage: auto& text = addComponent<Text>("Hi", 10, 10) ->
        template<typename T, typename... Args>
        T& addComponent(Args&&... args) {
            static_assert(
                std::is_base_of_v<UIComponent, T>,
                "T must inherit from UIComponent"
            );

            auto component = std::make_unique<T>(std::forward<Args>(args)...);
            T& reference = *component;

            components_.push_back(ComponentEntry{std::move(component), nullptr});
            finalizeRegistration(&reference);

            return reference;
        }

        //non-owning registration, usage: addComponent(&myStackComponent) ->
        void addComponent(UIComponent* component);

        void removeComponent(UIComponent* component);
        void update();
        void render(RendererOpts& renderer);
        void clear();

        //CSS helpers ->
        StyleManager& getStyleManager() {
            return styleManager_;
        }

        void applyStylesTo(UIComponent* component);
        void reloadStyles();

        std::size_t getComponentCount() const;
        bool contains(UIComponent* component) const;

    private:
        void finalizeRegistration(UIComponent* component);
        void detachComponent(UIComponent* component);

        std::vector<ComponentEntry> components_;
        StyleManager styleManager_;
    };

} // emgui
