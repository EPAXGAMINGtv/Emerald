//
// Created by epax on 22.11.26.
//

#include "emrldgui/style/StyleResolver.h"

#include "emrldgui/ui_component/UIComponent.h"

namespace emgui {

    namespace {

        int specificity(const CssRule& rule) {
            int result = 0;

            if (!rule.component.empty()) {
                ++result;
            }
            if (!rule.styleClass.empty()) {
                result += 10;
            }
            if (!rule.id.empty()) {
                result += 100;
            }

            return result;
        }

        bool matches(
            const CssRule& rule,
            const std::string& componentName,
            const std::string& styleClass,
            const std::string& id
        ) {
            if (!rule.component.empty() && rule.component != componentName) {
                return false;
            }
            if (!rule.styleClass.empty() && rule.styleClass != styleClass) {
                return false;
            }
            if (!rule.id.empty() && rule.id != id) {
                return false;
            }
            return true;
        }

    } // namespace

    StyleValues StyleResolver::resolve(const StyleSheet& sheet, const UIComponent& component) {
        return resolve(
            sheet,
            component.getComponentName(),
            component.getStyleClass(),
            component.getStyleId()
        );
    }

    StyleValues StyleResolver::resolve(
        const StyleSheet& sheet,
        const std::string& componentName,
        const std::string& styleClass,
        const std::string& id
    ) {
        StyleValues values;
        int currentSpecificity = -1;
        std::size_t currentOrder = 0;
        std::size_t order = 0;

        for (const CssRule& rule : sheet.getRules()) {
            if (!matches(rule, componentName, styleClass, id)) {
                ++order;
                continue;
            }

            const int ruleSpecificity = specificity(rule);

            if (ruleSpecificity < currentSpecificity ||
                (ruleSpecificity == currentSpecificity && order < currentOrder)) {
                ++order;
                continue;
            }

            currentSpecificity = ruleSpecificity;
            currentOrder = order;

            for (const CssDeclaration& declaration : rule.declarations) {
                values.set(declaration.property, declaration.value);
            }

            ++order;
        }

        return values;
    }

} // emgui
