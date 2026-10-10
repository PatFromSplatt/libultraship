#pragma once

#include <cstdint>
#include <string>

#include <imgui.h>

namespace Ship {

class Mobile {
  public:
    static void ImGuiProcessEvent(bool wantsTextInput);
    // App lifecycle (set from an SDL event watch). Rendering must not touch the GPU while
    // backgrounded: iOS drops background presents and the drawable pool starves.
    static void SetAppBackgrounded(bool backgrounded);
    static bool IsAppBackgrounded();
    // True exactly once after each transition into the background. The game's autosave polls
    // this from its own thread and saves on the next safe tick — the OS may jetsam a
    // backgrounded app at any moment, so "user switched away" must mean "progress is on disk".
    static bool ConsumeBackgroundSaveRequest();
    // The game's save routine for the background park. On iOS the render path blocks until
    // resume, so no game tick runs to consume the request above while the app is away. The
    // handler runs on the game thread between ticks and outside any ImGui frame, at most once
    // per background entry, inside a UIKit background task. nullptr unregisters.
    static void SetBackgroundSaveHandler(void (*fn)());
    // Renderer side of the above: true once per background entry.
    static bool ConsumeBackgroundParkEntry();
    static void RunBackgroundSaveHandler();
};
}; // namespace Ship
