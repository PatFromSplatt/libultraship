#if defined(__ANDROID__) || defined(__IOS__)
#include "ship/port/mobile/MobileImpl.h"
#include <SDL2/SDL.h>

#include <imgui_internal.h>

#include <atomic>

static bool isShowingVirtualKeyboard = true;
static std::atomic<bool> sAppBackgrounded{ false };

void Ship::Mobile::SetAppBackgrounded(bool backgrounded) {
    sAppBackgrounded.store(backgrounded);
}

bool Ship::Mobile::IsAppBackgrounded() {
    return sAppBackgrounded.load();
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
