// Created by epax on 22.11.26.

#pragma once

#include <SDL.h>

#include <map>
#include <string>
#include <vector>

namespace emgui {

    // Resolved, typed values for one component. Missing properties use defaults
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
        bool getBool(const std::string& property, bool fallback) const;
        float getFloat(const std::string& property, float fallback) const;
        SDL_Color getColor(
            const std::string& property,
            const SDL_Color& fallback
        ) const;
        std::vector<SDL_Color> getColors(
            const std::string& property,
            const std::vector<SDL_Color>& fallback = {}
        ) const;

        // Theme values are applied first, component values overwrite them
        void merge(const StyleValues& other);

    private:
        std::map<std::string, std::string> values_;
    };

} // emgui
