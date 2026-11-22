//
// Created by epax on 22.11.26.
//

#pragma once

#include <string>
#include <utility>
#include <vector>

namespace emgui {

    struct CssDeclaration {
        std::string property;
        std::string value;
    };

    struct CssRule {
        std::string selector;
        std::string component;
        std::string styleClass;
        std::string id;
        std::vector<CssDeclaration> declarations;
    };

    class StyleSheet {
    public:
        void addRule(CssRule rule) {
            rules_.push_back(std::move(rule));
        }

        const std::vector<CssRule>& getRules() const {
            return rules_;
        }

        void clear() {
            rules_.clear();
        }

    private:
        std::vector<CssRule> rules_;
    };

} // emgui
