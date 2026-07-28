#pragma once
#include <cstdint>
#include <atomic>

namespace Ship {
// State written by the touch overlay (SDL event thread / main thread) and the gyro shim
// (CoreMotion callback), merged once per frame into port 1's OSContPad by
// LUS::ControlDeck::WriteToOSContPad. Camera values are per-frame deltas in -1..1; the
// overlay drains its accumulators into them each Draw().
struct TouchControllerState {
    std::atomic<uint16_t> buttons{ 0 }; // BTN_* bitmask (libultra/controller.h)
    std::atomic<int8_t> stickX{ 0 };    // N64 stick range -128..127
    std::atomic<int8_t> stickY{ 0 };
    std::atomic<float> cameraX{ 0.0f }; // right-stick equivalent, -1..1
    std::atomic<float> cameraY{ 0.0f };
    std::atomic<float> gyroX{ 0.0f }; // device rotation rate, consumed via OSContPad gyro fields
    std::atomic<float> gyroY{ 0.0f };
    std::atomic<uint32_t> gyroSeq{ 0 }; // bumped per sensor sample; a stalled sensor must not
                                        // leave a stale rate applied forever

    static TouchControllerState& Instance() {
        static TouchControllerState sInstance;
        return sInstance;
    }
};
} // namespace Ship
