//
// Created by epax on 22.11.26.
//

#pragma once

#include <SDL.h>

#include <map>
#include <string>

namespace emgui {

    //resolved, typed values for one component -> missing properties use defaults
    class StyleValues {
    public:
        void set(std::string property, std::string value) {
            values_[std::move(property)] = std::move(value);
        }

        bool contains(const std::string& property) const {
            return values_.find(property) != values_.end();
        }

        std::string getString(
            const std::string& property,
            const std::string& fallback = {}
        ) const;

        int getInt(const std::string& property, int fallback) const;
        SDL_Color getColor(
            const std::string& property,
            const SDL_Color& fallback
        ) const;

    private:
        std::map<std::string, std::string> values_;
    };

} // emgui
