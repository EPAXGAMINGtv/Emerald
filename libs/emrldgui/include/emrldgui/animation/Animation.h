// Created by epax on 24.11.26.

#pragma once

#include <SDL.h>

#include <cstddef>
#include <cstdint>
#include <functional>

namespace emgui {

    // Small tween engine shared by all components. Tracks are ticked once per
    // frame by UIComponentManager::update().
    class Animation {
    public:
        using Handle = std::uint32_t;
        static constexpr Handle kInvalidHandle = 0;

        enum class Easing {
            Linear,
            EaseInQuad,
            EaseOutQuad,
            EaseInOutQuad,
            EaseOutCubic,
            EaseInOutCubic
        };

        // Runs a value transition from `from` to `to`. onUpdate is called with
        // the interpolated value on every frame, onComplete once at the end.
        static Handle run(
            float from,
            float to,
            Uint32 durationMs,
            std::function<void(float)> onUpdate,
            std::function<void()> onComplete = {},
            Easing easing = Easing::EaseInOutCubic
        );

        // Convenience wrapper that writes directly into a float. Only use for
        // members that outlive the animation; cancel() stops further writes.
        static Handle tween(
            float& value,
            float to,
            Uint32 durationMs,
            Easing easing = Easing::EaseInOutCubic,
            std::function<void()> onComplete = {}
        );

        static void cancel(Handle handle);
        static void cancelAll();
        static void tick(Uint32 nowMs);
        static std::size_t getActiveCount();

        static float ease(Easing easing, float progress);

        Animation() = delete;
    };

} // emgui
