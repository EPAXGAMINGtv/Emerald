// Created by epax on 22.11.26.

#include "emrldgui/style/StyleResolver.h"

#include "emrldgui/ui_component/UIComponent.h"

#include <algorithm>
#include <cctype>
#include <functional>
#include <map>
#include <vector>

namespace emgui {

    namespace {

        int specificity(const CssRule& rule) {
            int result = 0;

            if (!rule.component.empty() && rule.component != "*") {
                ++result;
            }
            if (!rule.styleClass.empty()) {
                result += 10;
            }
            if (!rule.id.empty()) {
                result += 100;
            }
            if (!rule.pseudoClass.empty()) {
                result += 10;
            }

            return result;
        }

        using StateCheck = std::function<bool(const std::string&)>;

        bool matches(
            const CssRule& rule,
            const std::string& componentName,
            const std::string& styleClass,
            const std::string& id,
            const StateCheck& hasState
        ) {
            if (!rule.pseudoClass.empty() &&
                (!hasState || !hasState(rule.pseudoClass))) {
                return false;
            }
            if (!rule.component.empty() && rule.component != "*" &&
                rule.component != componentName) {
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

        std::string trim(const std::string& value) {
            std::size_t first = 0;

            while (first < value.size() &&
                   std::isspace(static_cast<unsigned char>(value[first])) != 0) {
                ++first;
            }

            std::size_t last = value.size();

            while (last > first &&
                   std::isspace(static_cast<unsigned char>(value[last - 1])) != 0) {
                --last;
            }

            return value.substr(first, last - first);
        }

        // Supports var(--name) and var(--name, fallback), including nesting
        std::string substituteVariables(
            const std::string& value,
            const std::map<std::string, std::string>& variables,
            int depth = 0
        ) {
            if (depth > 8) {
                return value;
            }

            std::string result = value;
            std::size_t position = 0;

            while (position < result.size()) {
                const std::size_t start = result.find("var(", position);

                if (start == std::string::npos) {
                    break;
                }

                std::size_t end = start + 4;
                int nesting = 1;

                while (end < result.size() && nesting > 0) {
                    if (result[end] == '(') {
                        ++nesting;
                    } else if (result[end] == ')') {
                        --nesting;
                    }
                    ++end;
                }

                if (nesting != 0) {
                    break;
                }

                const std::string argument = result.substr(start + 4, end - start - 5);

                int argumentNesting = 0;
                std::size_t comma = std::string::npos;

                for (std::size_t index = 0; index < argument.size(); ++index) {
                    if (argument[index] == '(') {
                        ++argumentNesting;
                    } else if (argument[index] == ')') {
                        --argumentNesting;
                    } else if (argument[index] == ',' && argumentNesting == 0) {
                        comma = index;
                        break;
                    }
                }

                const std::string name = trim(
                    comma == std::string::npos ? argument : argument.substr(0, comma)
                );
                std::string replacement;

                const auto found = variables.find(name);

                if (found != variables.end()) {
                    replacement = found->second;
                } else if (comma != std::string::npos) {
                    replacement = trim(argument.substr(comma + 1));
                }

                replacement = substituteVariables(replacement, variables, depth + 1);
                result.replace(start, end - start, replacement);
                position = start + replacement.size();
            }

            return result;
        }

        StyleValues resolveSheet(
            const StyleSheet& sheet,
            const std::string& componentName,
            const std::string& styleClass,
            const std::string& id,
            const StateCheck& hasState
        ) {
            StyleValues values;
            std::map<std::string, std::string> variables;

            struct Match {
                const CssRule* rule = nullptr;
                int ruleSpecificity = 0;
                std::size_t order = 0;
            };

            std::vector<Match> matched;

            for (const CssRule& rule : sheet.getRules()) {
                if (!matches(rule, componentName, styleClass, id, hasState)) {
                    continue;
                }

                matched.push_back({&rule, specificity(rule), matched.size()});
            }

            std::stable_sort(
                matched.begin(),
                matched.end(),
                [](const Match& left, const Match& right) {
                    if (left.ruleSpecificity != right.ruleSpecificity) {
                        return left.ruleSpecificity < right.ruleSpecificity;
                    }
                    return left.order < right.order;
                }
            );

            for (const Match& match : matched) {
                for (const CssDeclaration& declaration : match.rule->declarations) {
                    if (declaration.property.starts_with("--")) {
                        variables[declaration.property] = substituteVariables(
                            declaration.value,
                            variables
                        );
                    }
                }
            }

            for (const Match& match : matched) {
                for (const CssDeclaration& declaration : match.rule->declarations) {
                    if (!declaration.property.starts_with("--")) {
                        values.set(
                            declaration.property,
                            substituteVariables(declaration.value, variables)
                        );
                    }
                }
            }

            return values;
        }

    } // namespace

    StyleValues StyleResolver::resolve(const StyleSheet& sheet, const UIComponent& component) {
        return resolveSheet(
            sheet,
            component.getComponentName(),
            component.getStyleClass(),
            component.getStyleId(),
            [&component](const std::string& state) {
                return component.hasState(state);
            }
        );
    }

    StyleValues StyleResolver::resolve(
        const StyleSheet& sheet,
        const std::string& componentName,
        const std::string& styleClass,
        const std::string& id
    ) {
        return resolveSheet(sheet, componentName, styleClass, id, {});
    }

} // emgui
