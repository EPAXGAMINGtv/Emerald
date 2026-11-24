// Created by epax on 24.11.26.

#include "emrldgui/ui_component/components/TextArea.h"

#include "emrldgui/style/StyleValues.h"
#include "spdlog/spdlog.h"

#include <algorithm>
#include <cctype>
#include <exception>
#include <limits>
#include <utility>

namespace emgui {

    namespace {

        bool isUtf8Continuation(unsigned char byte) {
            return (static_cast<unsigned int>(byte) & 0xC0U) == 0x80U;
        }

        std::size_t previousUtf8Start(const std::string& text, std::size_t position) {
            if (position == 0) {
                return 0;
            }

            std::size_t start = std::min(position, text.size()) - 1;

            while (start > 0 && isUtf8Continuation(
                static_cast<unsigned char>(text[start])
            )) {
                --start;
            }

            return start;
        }

        std::size_t nextUtf8End(const std::string& text, std::size_t position) {
            if (position >= text.size()) {
                return text.size();
            }

            std::size_t end = position + 1;

            while (end < text.size() && isUtf8Continuation(
                static_cast<unsigned char>(text[end])
            )) {
                ++end;
            }

            return end;
        }

        std::size_t clampToBoundary(const std::string& text, std::size_t position) {
            std::size_t result = std::min(position, text.size());

            while (result > 0 && result < text.size() &&
                   isUtf8Continuation(static_cast<unsigned char>(text[result]))) {
                --result;
            }

            return result;
        }

        std::size_t countCodepoints(const std::string& text) {
            std::size_t count = 0;

            for (const char character : text) {
                if (!isUtf8Continuation(static_cast<unsigned char>(character))) {
                    ++count;
                }
            }

            return count;
        }

        bool isWhitespace(char character) {
            return std::isspace(static_cast<unsigned char>(character)) != 0;
        }

        // Expand tabs and normalize Windows line endings before inserting text
        std::string sanitizeText(const std::string& value, int tabSize) {
            const std::size_t spaces = static_cast<std::size_t>(std::max(1, tabSize));
            std::string result;
            result.reserve(value.size());

            for (std::size_t index = 0; index < value.size(); ++index) {
                const char character = value[index];

                if (character == '\r') {
                    if (index + 1 < value.size() && value[index + 1] == '\n') {
                        ++index;
                    }
                    result.push_back('\n');
                } else if (character == '\t') {
                    result.append(spaces, ' ');
                } else {
                    result.push_back(character);
                }
            }

            return result;
        }

    } // namespace

    TextArea* TextArea::focusedArea_ = nullptr;

    TextArea::TextArea(
        std::string placeholder,
        int x,
        int y,
        int width,
        int height
    )
        : UIComponent(x, y),
          placeholder_(std::move(placeholder)),
          width_(std::max(0, width)),
          height_(std::max(0, height)) {
        setFocusable(true);

        surface_.color = {255, 255, 255, 255};
        surface_.borderColor = {192, 192, 192, 255};
        surface_.hasBorderColor = true;
        surface_.borderWidth = 1;
        surface_.radii = {8, 8, 8, 8};

        padding_.top = 10;
        padding_.right = 12;
        padding_.bottom = 10;
        padding_.left = 12;

        caretBlinkStart_ = SDL_GetTicks();
    }

    TextArea::~TextArea() {
        // Never touch the manager here: it may be tearing components down.
        if (focusedArea_ == this) {
            focusedArea_ = nullptr;
            SDL_StopTextInput();
        }

        if (window_ == nullptr || window_->getWindow() == nullptr) {
            return;
        }

        if (auto* manager = EventManager::getForWindow(window_->getWindow())) {
            manager->removeListener(mouseListener_);
            manager->removeListener(keyboardListener_);
            manager->removeListener(textInputListener_);
        }
    }

    void TextArea::onWindowAttached(Window& window) {
        if (mouseListener_ != EventManager::kInvalidListener) {
            return;
        }

        SDL_Window* sdlWindow = window.getWindow();

        if (sdlWindow == nullptr) {
            return;
        }

        mouseListener_ = EventManager::registerEvent(
            sdlWindow,
            EventMouse(sdlWindow),
            [this](EventMouse& event) {
                onMouseEvent(event);
            }
        );

        keyboardListener_ = EventManager::registerEvent(
            sdlWindow,
            EventKeyboard(sdlWindow),
            [this](EventKeyboard& event) {
                onKeyboardEvent(event);
            }
        );

        textInputListener_ = EventManager::registerEvent(
            sdlWindow,
            EventTextInput(sdlWindow),
            [this](EventTextInput& event) {
                onTextInputEvent(event);
            }
        );
    }

    void TextArea::onTextChanged(Action action) {
        if (action) {
            textChangedActions_.push_back(std::move(action));
        }
    }

    void TextArea::onEnter(Action action) {
        if (action) {
            enterActions_.push_back(std::move(action));
        }
    }

    void TextArea::onFocusGained(Action action) {
        if (action) {
            focusGainedActions_.push_back(std::move(action));
        }
    }

    void TextArea::onFocusLost(Action action) {
        if (action) {
            focusLostActions_.push_back(std::move(action));
        }
    }

    void TextArea::applyStyle(const StyleValues& values) {
        surface_.apply(values);
        textStyle_.apply(values);
        padding_.apply(values, 10);

        backgroundFocusColor_ = values.getColor(
            "background-focus-color",
            backgroundFocusColor_
        );
        const std::vector<SDL_Color> focusStops = values.getColors(
            "background-focus-gradient"
        );
        focusGradient_ = focusStops.size() >= 2 ? focusStops : std::vector<SDL_Color>{};

        disabledBackgroundColor_ = values.getColor(
            "disabled-background-color",
            disabledBackgroundColor_
        );
        borderFocusColor_ = values.getColor("border-focus-color", borderFocusColor_);
        disabledBorderColor_ = values.getColor(
            "disabled-border-color",
            disabledBorderColor_
        );

        textColor_ = values.getColor("text-color", textColor_);
        placeholderColor_ = values.getColor("placeholder-color", placeholderColor_);
        disabledTextColor_ = values.getColor("disabled-text-color", disabledTextColor_);
        caretColor_ = values.getColor("caret-color", caretColor_);
        selectionColor_ = values.getColor("selection-color", selectionColor_);
        selectionTextColor_ = values.getColor(
            "selection-text-color",
            selectionTextColor_
        );
        gutterColor_ = values.getColor("gutter-color", gutterColor_);
        scrollbarColor_ = values.getColor("scrollbar-color", scrollbarColor_);
        focusRingColor_ = values.getColor("focus-ring-color", focusRingColor_);
        focusRingWidth_ = std::max(
            0,
            values.getInt("focus-ring-width", focusRingWidth_)
        );

        caretWidth_ = std::max(1, values.getInt("caret-width", caretWidth_));
        caretBlinkMs_ = std::max(1, values.getInt("caret-blink-ms", caretBlinkMs_));

        fontSpec_.family = values.getString("font-family", fontSpec_.family);
        fontSpec_.size = std::max(1, values.getInt("font-size", fontSpec_.size));
        fontSpec_.style = values.getString("font-style", fontSpec_.style);

        lineHeight_ = std::max(
            fontSpec_.size + 2,
            values.getInt("line-height", fontSpec_.size + 4)
        );
        wordWrap_ = values.getBool("word-wrap", wordWrap_);
        tabSize_ = std::max(1, values.getInt("tab-size", tabSize_));
        showLineNumbers_ = values.getBool("show-line-numbers", showLineNumbers_);
        gutterWidth_ = std::max(0, values.getInt("gutter-width", gutterWidth_));

        // Cached row geometry depends on all of the values above
        visualLines_.clear();
        contentWidthPx_ = 0;

        spdlog::debug(
            "TextArea style: font={} size={} lineHeight={} wrap={}",
            fontSpec_.family,
            fontSpec_.size,
            lineHeight_,
            wordWrap_
        );
    }

    void TextArea::update() {
        if (!focused_ || !enabled_) {
            caretVisible_ = false;
            return;
        }

        const Uint32 elapsed = SDL_GetTicks() - caretBlinkStart_;
        caretVisible_ = (elapsed % static_cast<Uint32>(caretBlinkMs_)) <
                        static_cast<Uint32>(caretBlinkMs_ / 2);
    }

    void TextArea::render(RendererOpts& renderer) {
        const SDL_Rect rect{x_, y_, width_, height_};

        if (rect.w <= 0 || rect.h <= 0) {
            return;
        }

        SDL_Color background = surface_.color;
        SDL_Color border = surface_.hasBorderColor
            ? surface_.borderColor
            : disabledBorderColor_;
        SDL_Color foreground = textColor_;
        std::vector<SDL_Color> stops = surface_.gradient;

        if (focused_ && enabled_) {
            background = backgroundFocusColor_;
            border = borderFocusColor_;

            if (!focusGradient_.empty()) {
                stops = focusGradient_;
            }
        }

        if (!enabled_) {
            background = disabledBackgroundColor_;
            border = disabledBorderColor_;
            foreground = disabledTextColor_;
            stops.clear();
        }

        surface_.drawShadow(renderer, rect);
        surface_.drawBackgroundStops(renderer, rect, background, stops);
        surface_.drawBorderColor(renderer, rect, border);

        if (focused_ && enabled_ && focusRingWidth_ > 0 && focusRingColor_.a > 0) {
            renderer.drawRoundedRect(
                {
                    rect.x - focusRingWidth_,
                    rect.y - focusRingWidth_,
                    rect.w + 2 * focusRingWidth_,
                    rect.h + 2 * focusRingWidth_
                },
                surface_.maxRadius() + focusRingWidth_,
                focusRingColor_
            );
        }

        rebuildRows(renderer);

        const SDL_Rect inner = getInnerRect();

        if (inner.w <= 0 || inner.h <= 0) {
            return;
        }

        const int gutter = getGutterWidth();
        const int textX = getTextX();
        const bool hasSelection = selectionAnchor_ != caret_;
        const std::size_t selectionStart = std::min(selectionAnchor_, caret_);
        const std::size_t selectionEnd = std::max(selectionAnchor_, caret_);

        renderer.pushClipRect(inner);

        if (gutter > 0) {
            renderer.fillRect(
                {inner.x, inner.y, gutter, inner.h},
                withOpacity(gutterColor_, 0.25f)
            );
        }

        if (text_.empty()) {
            const SDL_Color placeholderInk = enabled_
                ? placeholderColor_
                : disabledTextColor_;
            textStyle_.draw(
                renderer,
                placeholder_,
                {textX - scrollX_, inner.y},
                placeholderInk,
                fontSpec_
            );
        } else {
            std::size_t previousLogicalLine =
                std::numeric_limits<std::size_t>::max();

            for (std::size_t rowIndex = 0; rowIndex < visualLines_.size(); ++rowIndex) {
                const VisualLine& row = visualLines_[rowIndex];
                const int rowY = inner.y +
                                 static_cast<int>(rowIndex) * lineHeight_ -
                                 scrollY_;

                if (rowY + lineHeight_ < inner.y) {
                    continue;
                }
                if (rowY > inner.y + inner.h) {
                    break;
                }

                if (gutter > 0 && row.logicalLine != previousLogicalLine) {
                    const std::string label = std::to_string(row.logicalLine + 1);
                    const SDL_Point labelSize = renderer.measureText(label, fontSpec_);
                    textStyle_.draw(
                        renderer,
                        label,
                        {inner.x + gutter - 6 - labelSize.x, rowY},
                        gutterColor_,
                        fontSpec_
                    );
                    previousLogicalLine = row.logicalLine;
                }

                const std::size_t rowSelectionStart = std::max(row.start, selectionStart);
                const std::size_t rowSelectionEnd = std::min(row.end, selectionEnd);

                if (hasSelection && rowSelectionEnd > rowSelectionStart) {
                    const int beforeWidth = measureRange(
                        renderer,
                        row.start,
                        rowSelectionStart
                    );
                    const int selectedWidth = measureRange(
                        renderer,
                        rowSelectionStart,
                        rowSelectionEnd
                    );

                    renderer.fillRect(
                        {
                            textX - scrollX_ + beforeWidth,
                            rowY,
                            std::max(1, selectedWidth),
                            lineHeight_
                        },
                        selectionColor_
                    );

                    drawSegment(
                        renderer,
                        row.start,
                        rowSelectionStart,
                        textX - scrollX_,
                        rowY,
                        foreground
                    );
                    drawSegment(
                        renderer,
                        rowSelectionStart,
                        rowSelectionEnd,
                        textX - scrollX_ + beforeWidth,
                        rowY,
                        selectionTextColor_
                    );
                    drawSegment(
                        renderer,
                        rowSelectionEnd,
                        row.end,
                        textX - scrollX_ + beforeWidth + selectedWidth,
                        rowY,
                        foreground
                    );
                } else {
                    drawSegment(
                        renderer,
                        row.start,
                        row.end,
                        textX - scrollX_,
                        rowY,
                        foreground
                    );
                }
            }
        }

        if (focused_ && enabled_ && caretVisible_) {
            int caretX = textX - scrollX_;
            int caretY = inner.y;

            if (!text_.empty() && !visualLines_.empty()) {
                const std::size_t caretRow = findCaretRow();
                const VisualLine& row = visualLines_[caretRow];
                caretX += measureRange(renderer, row.start, caret_);
                caretY += static_cast<int>(caretRow) * lineHeight_ - scrollY_;
            }

            renderer.fillRect(
                {caretX, caretY, caretWidth_, lineHeight_},
                caretColor_
            );
        }

        renderer.popClipRect();

        const int maxScrollY = getMaxScrollY();

        if (maxScrollY > 0) {
            const int thickness = 6;
            const int trackX = inner.x + inner.w - thickness;
            const int totalHeight = static_cast<int>(visualLines_.size()) * lineHeight_;
            const int thumbHeight = std::min(
                inner.h,
                std::max(12, inner.h * inner.h / std::max(1, totalHeight))
            );
            const int thumbY = inner.y +
                               (inner.h - thumbHeight) * scrollY_ / maxScrollY;

            renderer.fillRoundedRect(
                {trackX, thumbY, thickness, thumbHeight},
                3,
                scrollbarColor_
            );
        }

        if (!wordWrap_ && getMaxScrollX() > 0) {
            const int thickness = 6;
            const int trackY = inner.y + inner.h - thickness;
            const int thumbWidth = std::min(
                inner.w,
                std::max(12, inner.w * inner.w / std::max(1, contentWidthPx_))
            );
            const int thumbX = inner.x +
                               (inner.w - thumbWidth) * scrollX_ / getMaxScrollX();

            renderer.fillRoundedRect(
                {thumbX, trackY, thumbWidth, thickness},
                3,
                scrollbarColor_
            );
        }
    }

    bool TextArea::containsPoint(int x, int y) const {
        return x >= x_ && x < x_ + width_ && y >= y_ && y < y_ + height_;
    }

    SDL_Rect TextArea::getInnerRect() const {
        return {
            x_ + padding_.left,
            y_ + padding_.top,
            std::max(0, width_ - padding_.horizontal()),
            std::max(0, height_ - padding_.vertical())
        };
    }

    int TextArea::getGutterWidth() const {
        return showLineNumbers_ ? std::max(0, gutterWidth_) : 0;
    }

    int TextArea::getTextX() const {
        const SDL_Rect inner = getInnerRect();
        const int gutter = getGutterWidth();
        return inner.x + gutter + (gutter > 0 ? 6 : 0);
    }

    int TextArea::getViewportWidth(bool reserveScrollbar) const {
        const SDL_Rect inner = getInnerRect();
        const int gutter = getGutterWidth();
        int width = inner.w - gutter - (gutter > 0 ? 6 : 0);

        if (reserveScrollbar) {
            width -= 8;
        }

        return std::max(1, width);
    }

    int TextArea::getMaxScrollX() const {
        return std::max(0, contentWidthPx_ - viewportWidth_);
    }

    int TextArea::getMaxScrollY() const {
        const int totalHeight = static_cast<int>(visualLines_.size()) * lineHeight_;
        return std::max(0, totalHeight - viewportHeight_);
    }

    void TextArea::clampScroll() {
        scrollX_ = std::clamp(scrollX_, 0, getMaxScrollX());
        scrollY_ = std::clamp(scrollY_, 0, getMaxScrollY());
    }

    std::size_t TextArea::lineCount() const {
        return static_cast<std::size_t>(
            std::count(text_.begin(), text_.end(), '\n')
        ) + 1;
    }

    std::size_t TextArea::getLineStart(std::size_t line) const {
        if (line == 0) {
            return 0;
        }

        std::size_t seen = 0;

        for (std::size_t index = 0; index < text_.size(); ++index) {
            if (text_[index] == '\n') {
                ++seen;

                if (seen == line) {
                    return index + 1;
                }
            }
        }

        return text_.size();
    }

    std::size_t TextArea::getLineEnd(std::size_t line) const {
        const std::size_t start = getLineStart(line);
        const std::size_t newline = text_.find('\n', start);
        return newline == std::string::npos ? text_.size() : newline;
    }

    std::size_t TextArea::lineForOffset(std::size_t offset) const {
        const std::size_t limited = std::min(offset, text_.size());
        std::size_t line = 0;

        for (std::size_t index = 0; index < limited; ++index) {
            if (text_[index] == '\n') {
                ++line;
            }
        }

        return line;
    }

    std::pair<std::vector<TextArea::VisualLine>, int> TextArea::buildRows(
        RendererOpts& renderer,
        int availableWidth
    ) const {
        std::vector<VisualLine> rows;
        const int maxWidth = std::max(1, availableWidth);
        int widest = 0;

        for (std::size_t line = 0; line < lineCount(); ++line) {
            const std::size_t lineStart = getLineStart(line);
            const std::size_t lineEnd = getLineEnd(line);

            if (!wordWrap_ || lineEnd == lineStart) {
                rows.push_back({line, lineStart, lineEnd});
                widest = std::max(
                    widest,
                    measureRange(renderer, lineStart, lineEnd)
                );
                continue;
            }

            std::size_t rowStart = lineStart;

            while (rowStart < lineEnd) {
                int rowWidth = 0;
                std::size_t scan = rowStart;
                std::size_t lastBreak = rowStart;
                bool mustBreak = false;

                while (scan < lineEnd) {
                    const std::size_t next = nextUtf8End(text_, scan);
                    const int advance = measureRange(renderer, scan, next);

                    // Always keep at least one codepoint so wrapping terminates
                    if (rowWidth + advance > maxWidth && scan > rowStart) {
                        mustBreak = true;
                        break;
                    }

                    rowWidth += advance;
                    scan = next;

                    if (isWhitespace(text_[scan - 1])) {
                        lastBreak = scan;
                    }
                }

                if (!mustBreak) {
                    rows.push_back({line, rowStart, lineEnd});
                    widest = std::max(
                        widest,
                        measureRange(renderer, rowStart, lineEnd)
                    );
                    break;
                }

                const std::size_t split = lastBreak > rowStart ? lastBreak : scan;
                rows.push_back({line, rowStart, split});
                widest = std::max(
                    widest,
                    measureRange(renderer, rowStart, split)
                );
                rowStart = split;
            }
        }

        if (rows.empty()) {
            rows.push_back({0, 0, 0});
        }

        return {rows, widest};
    }

    void TextArea::rebuildRows(RendererOpts& renderer) {
        const SDL_Rect inner = getInnerRect();
        viewportHeight_ = std::max(1, inner.h);

        std::pair<std::vector<VisualLine>, int> built = buildRows(
            renderer,
            getViewportWidth(false)
        );
        visualLines_ = std::move(built.first);
        contentWidthPx_ = built.second;

        const int totalHeight = static_cast<int>(visualLines_.size()) * lineHeight_;

        if (totalHeight > viewportHeight_) {
            viewportWidth_ = getViewportWidth(true);
            built = buildRows(renderer, viewportWidth_);
            visualLines_ = std::move(built.first);
            contentWidthPx_ = built.second;
        } else {
            viewportWidth_ = getViewportWidth(false);
        }

        clampScroll();
    }

    std::size_t TextArea::findCaretRow() const {
        if (visualLines_.empty()) {
            return 0;
        }

        std::size_t found = 0;

        for (std::size_t index = 0; index < visualLines_.size(); ++index) {
            if (visualLines_[index].start <= caret_) {
                found = index;
            } else {
                break;
            }
        }

        return found;
    }

    std::size_t TextArea::getOffsetFromX(
        RendererOpts& renderer,
        const VisualLine& row,
        int x
    ) const {
        std::size_t best = row.start;
        int bestDistance = std::numeric_limits<int>::max();
        std::size_t position = row.start;

        while (true) {
            const int positionX = measureRange(renderer, row.start, position);
            const int distance = std::abs(positionX - x);

            if (distance < bestDistance) {
                bestDistance = distance;
                best = position;
            }

            if (positionX >= x || position >= row.end) {
                break;
            }

            position = nextUtf8End(text_, position);
        }

        return clampToBoundary(text_, best);
    }

    std::size_t TextArea::getOffsetFromPoint(
        RendererOpts& renderer,
        int x,
        int y
    ) const {
        if (visualLines_.empty()) {
            return caret_;
        }

        const SDL_Rect inner = getInnerRect();
        const int rowOffset = y - inner.y + scrollY_;
        int rowIndex = rowOffset / std::max(1, lineHeight_);
        rowIndex = std::clamp(
            rowIndex,
            0,
            static_cast<int>(visualLines_.size()) - 1
        );

        const VisualLine& row = visualLines_[static_cast<std::size_t>(rowIndex)];
        const int localX = x - getTextX() + scrollX_;

        return getOffsetFromX(renderer, row, localX);
    }

    int TextArea::measureRange(
        const RendererOpts& renderer,
        std::size_t start,
        std::size_t end
    ) const {
        if (end <= start || start >= text_.size()) {
            return 0;
        }

        const std::size_t limitedEnd = std::min(end, text_.size());
        const std::string part = textStyle_.prepare(
            text_.substr(start, limitedEnd - start)
        );
        const int base = renderer.measureText(part, fontSpec_).x;
        const int characters = static_cast<int>(
            countCodepoints(text_.substr(start, limitedEnd - start))
        );

        return base + textStyle_.letterSpacing * std::max(0, characters - 1);
    }

    void TextArea::drawSegment(
        RendererOpts& renderer,
        std::size_t start,
        std::size_t end,
        int x,
        int y,
        const SDL_Color& color
    ) const {
        if (end <= start || start >= text_.size()) {
            return;
        }

        const std::size_t limitedEnd = std::min(end, text_.size());
        textStyle_.draw(
            renderer,
            text_.substr(start, limitedEnd - start),
            {x, y},
            color,
            fontSpec_
        );
    }

    void TextArea::ensureCaretVisible(RendererOpts& renderer) {
        rebuildRows(renderer);

        if (visualLines_.empty()) {
            return;
        }

        const std::size_t caretRow = findCaretRow();
        const int rowY = static_cast<int>(caretRow) * lineHeight_;

        if (rowY < scrollY_) {
            scrollY_ = rowY;
        } else if (rowY + lineHeight_ > scrollY_ + viewportHeight_) {
            scrollY_ = rowY + lineHeight_ - viewportHeight_;
        }

        const VisualLine& row = visualLines_[caretRow];
        const int caretX = measureRange(renderer, row.start, caret_);

        if (caretX < scrollX_) {
            scrollX_ = caretX;
        } else if (caretX + caretWidth_ > scrollX_ + viewportWidth_) {
            scrollX_ = caretX + caretWidth_ - viewportWidth_;
        }

        clampScroll();
    }

    void TextArea::moveCaretVertically(
        RendererOpts& renderer,
        int rows,
        bool extend
    ) {
        rebuildRows(renderer);

        if (visualLines_.empty()) {
            return;
        }

        const std::size_t currentRow = findCaretRow();
        const int desiredX = measureRange(
            renderer,
            visualLines_[currentRow].start,
            caret_
        );
        const int target = std::clamp(
            static_cast<int>(currentRow) + rows,
            0,
            static_cast<int>(visualLines_.size()) - 1
        );
        const VisualLine& row = visualLines_[static_cast<std::size_t>(target)];

        setCaret(getOffsetFromX(renderer, row, desiredX), extend);
    }

    void TextArea::moveCaretHomeEnd(
        RendererOpts& renderer,
        bool end,
        bool extend
    ) {
        rebuildRows(renderer);

        if (wordWrap_ && !visualLines_.empty()) {
            const VisualLine& row = visualLines_[findCaretRow()];
            setCaret(end ? row.end : row.start, extend);
            return;
        }

        const std::size_t line = lineForOffset(caret_);
        setCaret(end ? getLineEnd(line) : getLineStart(line), extend);
    }

    void TextArea::resetCaretBlink() {
        caretBlinkStart_ = SDL_GetTicks();
        caretVisible_ = true;
    }

    void TextArea::setCaret(std::size_t offset, bool extendSelection) {
        caret_ = clampToBoundary(text_, std::min(offset, text_.size()));

        if (!extendSelection) {
            selectionAnchor_ = caret_;
        }

        resetCaretBlink();
    }

    bool TextArea::replaceRange(
        std::size_t start,
        std::size_t end,
        const std::string& value
    ) {
        const std::size_t boundedStart = clampToBoundary(text_, start);
        const std::size_t boundedEnd = std::max(
            boundedStart,
            clampToBoundary(text_, end)
        );

        std::string next = text_;
        next.replace(boundedStart, boundedEnd - boundedStart, value);

        if (next == text_) {
            caret_ = boundedStart + value.size();
            selectionAnchor_ = caret_;
            resetCaretBlink();
            return false;
        }

        pushUndo();
        text_ = std::move(next);
        caret_ = boundedStart + value.size();
        selectionAnchor_ = caret_;
        resetCaretBlink();
        notifyTextChanged();
        return true;
    }

    void TextArea::replaceSelection(const std::string& value) {
        replaceRange(
            std::min(selectionAnchor_, caret_),
            std::max(selectionAnchor_, caret_),
            value
        );
    }

    void TextArea::deleteSelection() {
        if (selectionAnchor_ != caret_) {
            replaceSelection({});
        }
    }

    void TextArea::insertText(const std::string& value) {
        if (value.empty() || !enabled_ || readOnly_) {
            return;
        }

        replaceSelection(sanitizeText(value, tabSize_));
    }

    void TextArea::deleteBeforeCaret() {
        if (readOnly_) {
            return;
        }

        if (selectionAnchor_ != caret_) {
            deleteSelection();
            return;
        }

        if (caret_ == 0) {
            return;
        }

        replaceRange(previousUtf8Start(text_, caret_), caret_, {});
    }

    void TextArea::deleteAtCaret() {
        if (readOnly_) {
            return;
        }

        if (selectionAnchor_ != caret_) {
            deleteSelection();
            return;
        }

        if (caret_ >= text_.size()) {
            return;
        }

        replaceRange(caret_, nextUtf8End(text_, caret_), {});
    }

    bool TextArea::copy() {
        if (selectionAnchor_ == caret_) {
            return false;
        }

        const std::size_t start = std::min(selectionAnchor_, caret_);
        const std::size_t end = std::max(selectionAnchor_, caret_);
        return SDL_SetClipboardText(
            text_.substr(start, end - start).c_str()
        ) == 0;
    }

    bool TextArea::cut() {
        if (readOnly_ || !copy()) {
            return false;
        }

        deleteSelection();
        return true;
    }

    bool TextArea::paste() {
        if (readOnly_) {
            return false;
        }

        char* clipboard = SDL_GetClipboardText();

        if (clipboard == nullptr) {
            return false;
        }

        const std::string pasted = sanitizeText(clipboard, tabSize_);
        SDL_free(clipboard);

        if (pasted.empty()) {
            return false;
        }

        replaceSelection(pasted);
        return true;
    }

    void TextArea::pushUndo() {
        undoStack_.push_back({text_, caret_, selectionAnchor_});

        if (undoStack_.size() > 100) {
            undoStack_.erase(undoStack_.begin());
        }

        redoStack_.clear();
    }

    void TextArea::undo() {
        if (readOnly_ || undoStack_.empty()) {
            return;
        }

        const TextSnapshot snapshot = undoStack_.back();
        undoStack_.pop_back();
        redoStack_.push_back({text_, caret_, selectionAnchor_});

        text_ = snapshot.text;
        caret_ = clampToBoundary(text_, snapshot.caret);
        selectionAnchor_ = clampToBoundary(text_, snapshot.anchor);
        resetCaretBlink();
        notifyTextChanged();
    }

    void TextArea::redo() {
        if (readOnly_ || redoStack_.empty()) {
            return;
        }

        const TextSnapshot snapshot = redoStack_.back();
        redoStack_.pop_back();
        undoStack_.push_back({text_, caret_, selectionAnchor_});

        text_ = snapshot.text;
        caret_ = clampToBoundary(text_, snapshot.caret);
        selectionAnchor_ = clampToBoundary(text_, snapshot.anchor);
        resetCaretBlink();
        notifyTextChanged();
    }

    void TextArea::selectAll() {
        caret_ = text_.size();
        selectionAnchor_ = 0;
        resetCaretBlink();
    }

    void TextArea::notifyTextChanged() {
        runActions(textChangedActions_);
    }

    void TextArea::runActions(const std::vector<Action>& actions) {
        const auto snapshot = actions;

        for (const Action& action : snapshot) {
            try {
                action(*this);
            } catch (const std::exception& error) {
                spdlog::error("TextArea action failed: {}", error.what());
            }
        }
    }

    void TextArea::setText(std::string text) {
        replaceRange(0, text_.size(), sanitizeText(text, tabSize_));
    }

    std::size_t TextArea::getLineCount() const {
        return lineCount();
    }

    std::size_t TextArea::getCaretLine() const {
        return lineForOffset(caret_);
    }

    std::size_t TextArea::getCaretColumn() const {
        const std::size_t lineStart = getLineStart(lineForOffset(caret_));
        std::size_t column = 0;

        for (std::size_t index = lineStart; index < caret_ && index < text_.size();
             ++index) {
            if (!isUtf8Continuation(static_cast<unsigned char>(text_[index]))) {
                ++column;
            }
        }

        return column;
    }

    void TextArea::setEnabled(bool enabled) {
        enabled_ = enabled;
        setDisabled(!enabled);

        if (!enabled_) {
            setFocused(false);
        }
    }

    void TextArea::setFocused(bool focused) {
        if (focused && (!enabled_ || isDisabled())) {
            return;
        }

        if (focused_ == focused) {
            return;
        }

        focused_ = focused;
        UIComponent::setFocused(focused);

        if (focused) {
            if (focusedArea_ != nullptr && focusedArea_ != this) {
                focusedArea_->setFocused(false);
            }

            focusedArea_ = this;
            resetCaretBlink();
            SDL_StartTextInput();

            if (window_ != nullptr && window_->getWindow() != nullptr) {
                window_->getUIComponentManager().setFocusedComponent(this);
            }

            runActions(focusGainedActions_);
            return;
        }

        caretVisible_ = false;
        selectingWithMouse_ = false;

        if (focusedArea_ == this) {
            focusedArea_ = nullptr;
        }

        SDL_StopTextInput();

        if (window_ != nullptr && window_->getWindow() != nullptr) {
            UIComponentManager& manager = window_->getUIComponentManager();

            if (manager.getFocusedComponent() == this) {
                manager.setFocusedComponent(nullptr);
            }
        }

        runActions(focusLostActions_);
    }

    void TextArea::onMouseEvent(EventMouse& event) {
        if (!enabled_ || window_ == nullptr) {
            return;
        }

        RendererOpts& renderer = window_->getRendererOpts();

        if (event.isWheel()) {
            int pointerX = 0;
            int pointerY = 0;
            SDL_GetMouseState(&pointerX, &pointerY);

            if (!containsPoint(pointerX, pointerY) && !focused_) {
                return;
            }

            const int deltaX = event.getScrollX();
            const int deltaY = event.getScrollY();

            if (deltaX == 0 && deltaY == 0) {
                return;
            }

            const int step = std::max(1, lineHeight_ * 3);
            rebuildRows(renderer);
            scrollX_ += deltaX * step;
            scrollY_ -= deltaY * step;
            clampScroll();
            event.handled = true;
            return;
        }

        const int x = event.getX();
        const int y = event.getY();

        if (event.isPressed()) {
            if (event.getButton() != SDL_BUTTON_LEFT) {
                return;
            }

            const bool inside = containsPoint(x, y);
            setFocused(inside);

            if (!inside || !focused_) {
                return;
            }

            rebuildRows(renderer);
            const bool extend = (SDL_GetModState() & KMOD_SHIFT) != 0;
            setCaret(getOffsetFromPoint(renderer, x, y), extend);
            selectingWithMouse_ = true;
            event.handled = true;
            return;
        }

        if (event.isMoved() && selectingWithMouse_) {
            rebuildRows(renderer);
            caret_ = getOffsetFromPoint(renderer, x, y);
            resetCaretBlink();
            event.handled = true;
            return;
        }

        if (event.isReleased() && selectingWithMouse_) {
            selectingWithMouse_ = false;
            event.handled = true;
        }
    }

    void TextArea::onKeyboardEvent(EventKeyboard& event) {
        if (!focused_ || !enabled_ || !event.isPressed() || window_ == nullptr) {
            return;
        }

        const SDL_Keymod modifiers = event.getModifiers();
        const bool command = (modifiers & (KMOD_CTRL | KMOD_GUI)) != 0;
        const bool shift = (modifiers & KMOD_SHIFT) != 0;
        const SDL_Keycode key = event.getKey();
        bool consumed = true;

        if (command) {
            switch (key) {
                case SDLK_a:
                    selectAll();
                    break;
                case SDLK_c:
                    copy();
                    break;
                case SDLK_x:
                    cut();
                    break;
                case SDLK_v:
                    paste();
                    break;
                case SDLK_z:
                    if (shift) {
                        redo();
                    } else {
                        undo();
                    }
                    break;
                case SDLK_y:
                    redo();
                    break;
                case SDLK_HOME:
                    setCaret(0, shift);
                    break;
                case SDLK_END:
                    setCaret(text_.size(), shift);
                    break;
                default:
                    consumed = false;
                    break;
            }
        }

        if (!consumed) {
            switch (key) {
                case SDLK_LEFT:
                    if (caret_ > 0) {
                        setCaret(previousUtf8Start(text_, caret_), shift);
                    }
                    break;
                case SDLK_RIGHT:
                    if (caret_ < text_.size()) {
                        setCaret(nextUtf8End(text_, caret_), shift);
                    }
                    break;
                case SDLK_UP:
                    moveCaretVertically(window_->getRendererOpts(), -1, shift);
                    break;
                case SDLK_DOWN:
                    moveCaretVertically(window_->getRendererOpts(), 1, shift);
                    break;
                case SDLK_PAGEUP:
                    moveCaretVertically(
                        window_->getRendererOpts(),
                        -std::max(1, viewportHeight_ / std::max(1, lineHeight_)),
                        shift
                    );
                    break;
                case SDLK_PAGEDOWN:
                    moveCaretVertically(
                        window_->getRendererOpts(),
                        std::max(1, viewportHeight_ / std::max(1, lineHeight_)),
                        shift
                    );
                    break;
                case SDLK_HOME:
                    moveCaretHomeEnd(window_->getRendererOpts(), false, shift);
                    break;
                case SDLK_END:
                    moveCaretHomeEnd(window_->getRendererOpts(), true, shift);
                    break;
                case SDLK_BACKSPACE:
                    deleteBeforeCaret();
                    break;
                case SDLK_DELETE:
                    deleteAtCaret();
                    break;
                case SDLK_TAB:
                    if (!readOnly_) {
                        insertText(
                            std::string(static_cast<std::size_t>(tabSize_), ' ')
                        );
                    }
                    break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER:
                    if (!readOnly_) {
                        insertText("\n");
                    }
                    runActions(enterActions_);
                    break;
                case SDLK_ESCAPE:
                    setFocused(false);
                    break;
                default:
                    consumed = false;
                    break;
            }
        }

        if (!consumed) {
            return;
        }

        event.handled = true;

        if (focused_ && enabled_) {
            ensureCaretVisible(window_->getRendererOpts());
        }
    }

    void TextArea::onTextInputEvent(EventTextInput& event) {
        if (!focused_ || !enabled_ || readOnly_ || window_ == nullptr) {
            return;
        }

        std::string filtered;

        // Control characters are handled as key events (especially Return)
        for (const char character : event.getText()) {
            const auto byte = static_cast<unsigned int>(
                static_cast<unsigned char>(character)
            );

            if (byte < 0x20U || byte == 0x7FU) {
                continue;
            }

            filtered.push_back(character);
        }

        if (filtered.empty()) {
            return;
        }

        insertText(filtered);
        event.handled = true;
        ensureCaretVisible(window_->getRendererOpts());
    }

} // emgui
