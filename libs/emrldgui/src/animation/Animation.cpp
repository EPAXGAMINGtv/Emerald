// Created by epax on 24.11.26.

#include "emrldgui/animation/Animation.h"

#include <algorithm>
#include <cmath>
#include <memory>
#include <utility>
#include <vector>

namespace emgui {

    namespace {

        struct Track {
            Animation::Handle handle = Animation::kInvalidHandle;
            float from = 0.0f;
            float to = 0.0f;
            Uint32 startMs = 0;
            Uint32 durationMs = 0;
            Animation::Easing easing = Animation::Easing::Linear;
            std::function<void(float)> onUpdate;
            std::function<void()> onComplete;
            bool finished = false;
        };

        std::vector<std::unique_ptr<Track>>& tracks() {
            static std::vector<std::unique_ptr<Track>> active;
            return active;
        }

        Animation::Handle nextHandle() {
            static Animation::Handle current = Animation::kInvalidHandle;
            ++current;

            if (current == Animation::kInvalidHandle) {
                ++current;
            }

            return current;
        }

        float clamp01(float value) {
            if (value < 0.0f) {
                return 0.0f;
            }
            if (value > 1.0f) {
                return 1.0f;
            }
            return value;
        }

        float cubic(float value) {
            return value * value * value;
        }

    } // namespace

    float Animation::ease(Easing easing, float progress) {
        const float t = clamp01(progress);

        switch (easing) {
            case Easing::Linear:
                return t;
            case Easing::EaseInQuad:
                return t * t;
            case Easing::EaseOutQuad:
                return t * (2.0f - t);
            case Easing::EaseInOutQuad:
                return t < 0.5f ? 2.0f * t * t : -1.0f + (4.0f - 2.0f * t) * t;
            case Easing::EaseOutCubic:
                return 1.0f - cubic(1.0f - t);
            case Easing::EaseInOutCubic:
                return t < 0.5f
                    ? 4.0f * cubic(t)
                    : 1.0f - cubic(-2.0f * t + 2.0f) / 2.0f;
            default:
                return t;
        }
    }

    Animation::Handle Animation::run(
        float from,
        float to,
        Uint32 durationMs,
        std::function<void(float)> onUpdate,
        std::function<void()> onComplete,
        Easing easing
    ) {
        const Handle handle = nextHandle();
        auto track = std::make_unique<Track>();
        track->handle = handle;
        track->from = from;
        track->to = to;
        track->startMs = SDL_GetTicks();
        track->durationMs = durationMs;
        track->easing = easing;
        track->onUpdate = std::move(onUpdate);
        track->onComplete = std::move(onComplete);

        tracks().push_back(std::move(track));
        return handle;
    }

    Animation::Handle Animation::tween(
        float& value,
        float to,
        Uint32 durationMs,
        Easing easing,
        std::function<void()> onComplete
    ) {
        float* target = &value;

        return run(
            value,
            to,
            durationMs,
            [target](float interpolated) noexcept {
                *target = interpolated;
            },
            std::move(onComplete),
            easing
        );
    }

    void Animation::cancel(Handle handle) {
        for (const auto& track : tracks()) {
            if (track->handle == handle) {
                track->finished = true;
                return;
            }
        }
    }

    void Animation::cancelAll() {
        for (const auto& track : tracks()) {
            track->finished = true;
        }
    }

    void Animation::tick(Uint32 nowMs) {
        if (tracks().empty()) {
            return;
        }

        for (const auto& track : tracks()) {
            if (track->finished) {
                continue;
            }

            // Copy callbacks before invoking them: a callback may add or
            // cancel tracks and reallocate the owning vector.
            const auto onUpdate = track->onUpdate;
            const auto onComplete = track->onComplete;
            const float from = track->from;
            const float to = track->to;
            const Uint32 start = track->startMs;
            const Uint32 duration = track->durationMs;
            const Easing easing = track->easing;

            if (duration == 0) {
                if (onUpdate) {
                    onUpdate(to);
                }
                track->finished = true;
                if (onComplete) {
                    onComplete();
                }
                continue;
            }

            // Uint32 wraparound is handled by unsigned subtraction
            const Uint32 elapsed = nowMs - start;
            const float progress = clamp01(
                static_cast<float>(elapsed) / static_cast<float>(duration)
            );
            const float eased = ease(easing, progress);
            const float value = from + (to - from) * eased;

            if (onUpdate) {
                onUpdate(value);
            }

            if (progress >= 1.0f) {
                track->finished = true;
                if (onComplete) {
                    onComplete();
                }
            }
        }

        tracks().erase(
            std::remove_if(
                tracks().begin(),
                tracks().end(),
                [](const std::unique_ptr<Track>& track) {
                    return track->finished;
                }
            ),
            tracks().end()
        );
    }

    std::size_t Animation::getActiveCount() {
        std::size_t count = 0;

        for (const auto& track : tracks()) {
            if (!track->finished) {
                ++count;
            }
        }

        return count;
    }

} // emgui
