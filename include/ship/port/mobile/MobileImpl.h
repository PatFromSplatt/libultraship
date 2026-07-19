#pragma once

#include <cstdint>
#include <string>

#include <imgui.h>

namespace Ship {

class Mobile {
  public:
    static void ImGuiProcessEvent(bool wantsTextInput);
    // App lifecycle (set from an SDL event watch; SDL_APP_* events arrive during the OS
    // callback on mobile). Rendering must not touch the GPU while backgrounded.
    static void SetAppBackgrounded(bool backgrounded);
    static bool IsAppBackgrounded();
};
}; // namespace Ship
