#if defined(__ANDROID__) || defined(__IOS__)
#include "ship/port/mobile/MobileImpl.h"
#include <SDL2/SDL.h>

#include <imgui_internal.h>

#include <atomic>

static bool isShowingVirtualKeyboard = true;
static std::atomic<bool> sAppBackgrounded{ false };
static std::atomic<bool> sBackgroundSaveRequested{ false };

void Ship::Mobile::SetAppBackgrounded(bool backgrounded) {
    if (backgrounded && !sAppBackgrounded.load()) {
        // Request-once per transition; the game consumes it on its next tick. Deliberately not
        // saved here: this runs inside the OS lifecycle callback, and save code belongs to the
        // game thread.
        sBackgroundSaveRequested.store(true);
    }
    sAppBackgrounded.store(backgrounded);
}

bool Ship::Mobile::IsAppBackgrounded() {
    return sAppBackgrounded.load();
}

bool Ship::Mobile::ConsumeBackgroundSaveRequest() {
    return sBackgroundSaveRequested.exchange(false);
}

void Ship::Mobile::ImGuiProcessEvent(bool wantsTextInput) {
    ImGuiInputTextState* state = ImGui::GetInputTextState(ImGui::GetActiveID());

    if (wantsTextInput) {
        if (!isShowingVirtualKeyboard) {
            state->ClearText();

            isShowingVirtualKeyboard = true;
            SDL_StartTextInput();
        }
    } else {
        if (isShowingVirtualKeyboard) {
            isShowingVirtualKeyboard = false;
            SDL_StopTextInput();
        }
    }
}
#endif
