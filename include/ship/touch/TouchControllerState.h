#pragma once
#include <cstdint>
#include <atomic>

namespace Ship {
// State written by the touch overlay (SDL event thread / main thread) and the gyro shim
// (CoreMotion callback), merged once per game tick into port 1's OSContPad by
// LUS::ControlDeck::WriteToOSContPad, which drains the camera accumulators and button latches.
// The overlay only sees finger events once per tick but renders several frames per tick, so
// anything per-event must be held here until that read, never reset per rendered frame.
struct TouchControllerState {
    std::atomic<uint16_t> buttons{ 0 };       // BTN_* bitmask (libultra/controller.h)
    std::atomic<uint16_t> pressedLatch{ 0 };  // BTN_* bits that went down since the last pad read
    std::atomic<uint16_t> releasedLatch{ 0 }; // BTN_* bits that went up since the last pad read
    std::atomic<int8_t> stickX{ 0 };          // N64 stick range -128..127
    std::atomic<int8_t> stickY{ 0 };
    std::atomic<float> camAccumX{ 0.0f }; // right-stick drag since the last pad read, clamped to -1..1 there
    std::atomic<float> camAccumY{ 0.0f };
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
