//
// Created by epax on 22.11.26.
//

#pragma once

#include "StyleSheet.h"
#include "StyleValues.h"

#include <string>

namespace emgui {

    class UIComponent;

    //matches CSS rules against a component's type/class/id and applies specificity -> later rules win on ties
    class StyleResolver {
    public:
        static StyleValues resolve(const StyleSheet& sheet, const UIComponent& component);
        static StyleValues resolve(
            const StyleSheet& sheet,
            const std::string& componentName,
            const std::string& styleClass,
            const std::string& id
        );
    };

} // emgui
