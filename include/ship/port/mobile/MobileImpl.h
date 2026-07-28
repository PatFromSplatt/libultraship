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
};
}; // namespace Ship
