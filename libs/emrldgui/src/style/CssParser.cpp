// Created by epax on 22.11.26.

#include "emrldgui/style/CssParser.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iterator>
#include <utility>

#include "spdlog/spdlog.h"

namespace emgui {

    namespace {

        bool isSpace(char value) {
            return std::isspace(static_cast<unsigned char>(value)) != 0;
        }

        char toLower(char value) {
            return static_cast<char>(std::tolower(static_cast<unsigned char>(value)));
        }

        std::string lower(std::string_view value) {
            std::string result(value);
            std::transform(result.begin(), result.end(), result.begin(), toLower);
            return result;
        }

        std::string trim(std::string_view value) {
            std::size_t first = 0;
            while (first < value.size() && isSpace(value[first])) {
                ++first;
            }

            std::size_t last = value.size();
            while (last > first && isSpace(value[last - 1])) {
                --last;
            }

            return std::string(value.substr(first, last - first));
        }

        std::string stripComments(std::string_view source) {
            std::string result;
            result.reserve(source.size());

            bool inComment = false;

            for (std::size_t index = 0; index < source.size(); ++index) {
                const bool hasNext = index + 1 < source.size();

                if (inComment) {
                    if (hasNext && source[index] == '*' && source[index + 1] == '/') {
                        inComment = false;
                        ++index;
                    }
                    continue;
                }

                if (hasNext && source[index] == '/' && source[index + 1] == '*') {
                    inComment = true;
                    ++index;
                    continue;
                }

                result.push_back(source[index]);
            }

            return result;
        }

        CssRule parseSelector(std::string_view selector) {
            CssRule rule;
            rule.selector = trim(selector);

            if (rule.selector.empty()) {
                return rule;
            }

            // Pseudo-classes: Button:hover, #startButton:focus, Panel.selected
            const std::size_t colon = rule.selector.find(':');
            if (colon != std::string_view::npos) {
                rule.pseudoClass = lower(
                    trim(std::string_view(rule.selector).substr(colon + 1))
                );
                rule.selector = trim(
                    std::string_view(rule.selector).substr(0, colon)
                );
                if (rule.selector.empty()) {
                    return rule;
                }
            }

            if (rule.selector == "*") {
                rule.component = "*";
                return rule;
            }

            if (rule.selector.front() == '#') {
                rule.id = trim(std::string_view(rule.selector).substr(1));
                return rule;
            }

            const std::size_t dot = rule.selector.find('.');

            if (dot == std::string::npos) {
                rule.component = rule.selector;
            } else {
                rule.component = trim(std::string_view(rule.selector).substr(0, dot));
                rule.styleClass = trim(std::string_view(rule.selector).substr(dot + 1));
            }

            return rule;
        }

        void parseDeclarations(std::string_view body, CssRule& rule) {
            std::size_t position = 0;

            while (position < body.size()) {
                const std::size_t semicolon = body.find(';', position);
                const std::size_t end = semicolon == std::string_view::npos
                    ? body.size()
                    : semicolon;

                const std::string declaration = trim(body.substr(position, end - position));

                if (!declaration.empty()) {
                    const std::size_t colon = declaration.find(':');

                    if (colon == std::string::npos) {
                        spdlog::warn("CSS: ignoring malformed declaration '{}'", declaration);
                    } else {
                        const std::string property = lower(
                            trim(std::string_view(declaration).substr(0, colon))
                        );
                        const std::string value = trim(
                            std::string_view(declaration).substr(colon + 1)
                        );

                        if (property.empty() || value.empty()) {
                            spdlog::warn("CSS: ignoring empty declaration '{}'", declaration);
                        } else {
                            rule.declarations.push_back(CssDeclaration{property, value});
                        }
                    }
                }

                if (semicolon == std::string_view::npos) {
                    break;
                }

                position = semicolon + 1;
            }
        }

    } // namespace

    StyleSheet CssParser::parseFile(const std::filesystem::path& path) {
        std::ifstream input(path);

        if (!input.is_open()) {
            spdlog::error("Could not open CSS file {}", path.string());
            return {};
        }

        const std::string source{
            std::istreambuf_iterator<char>(input),
            std::istreambuf_iterator<char>()
        };

        return parseString(source);
    }

    StyleSheet CssParser::parseString(std::string_view source) {
        StyleSheet sheet;
        const std::string clean = stripComments(source);

        std::size_t position = 0;

        while (position < clean.size()) {
            const std::size_t opening = clean.find('{', position);

            if (opening == std::string::npos) {
                if (!trim(std::string_view(clean).substr(position)).empty()) {
                    spdlog::warn("CSS: ignoring trailing text without a rule body");
                }
                break;
            }

            const std::size_t closing = clean.find('}', opening + 1);

            if (closing == std::string::npos) {
                spdlog::warn("CSS: rule starting at {} has no closing brace", opening);
                break;
            }

            CssRule rule = parseSelector(
                std::string_view(clean).substr(position, opening - position)
            );

            parseDeclarations(
                std::string_view(clean).substr(opening + 1, closing - opening - 1),
                rule
            );

            if (!rule.selector.empty() && !rule.declarations.empty()) {
                sheet.addRule(std::move(rule));
            }

            position = closing + 1;
        }

        return sheet;
    }

} // emgui
