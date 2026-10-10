#if defined(__ANDROID__) || defined(__IOS__)
#include "ship/port/mobile/MobileImpl.h"
#include <SDL2/SDL.h>

#include <imgui_internal.h>

#include <atomic>

static bool isShowingVirtualKeyboard = true;
static std::atomic<bool> sAppBackgrounded{ false };
static std::atomic<bool> sBackgroundSaveRequested{ false };
static std::atomic<bool> sBackgroundParkPending{ false };
static std::atomic<void (*)()> sBackgroundSaveHandler{ nullptr };

void Ship::Mobile::SetAppBackgrounded(bool backgrounded) {
    if (backgrounded && !sAppBackgrounded.load()) {
        // Request-once per transition; the game consumes it on its next tick. Deliberately not
        // saved here: this runs inside the OS lifecycle callback, and save code belongs to the
        // game thread.
        sBackgroundSaveRequested.store(true);
        sBackgroundParkPending.store(true);
    }
    sAppBackgrounded.store(backgrounded);
}

bool Ship::Mobile::IsAppBackgrounded() {
    return sAppBackgrounded.load();
}

bool Ship::Mobile::ConsumeBackgroundSaveRequest() {
    return sBackgroundSaveRequested.exchange(false);
}

void Ship::Mobile::SetBackgroundSaveHandler(void (*fn)()) {
    sBackgroundSaveHandler.store(fn);
}

bool Ship::Mobile::ConsumeBackgroundParkEntry() {
    return sBackgroundParkPending.exchange(false);
}

void Ship::Mobile::RunBackgroundSaveHandler() {
    void (*handler)() = sBackgroundSaveHandler.load();
    if (handler != nullptr) {
        handler();
    }
}

void Ship::Mobile::ImGuiProcessEvent(bool wantsTextInput) {
    ImGuiInputTextState* state = ImGui::GetInputTextState(ImGui::GetActiveID());
    // io.WantTextInput is the last NewFrame's verdict; the touch-scroll latch may have cleared
    // ActiveId since, so only a live text field may raise the keyboard.
    const bool wantKeyboard = wantsTextInput && state != nullptr;
    if (wantKeyboard) {
        // Hold the keyboard until the press lifts: a press can still turn into a scroll.
        if (!isShowingVirtualKeyboard && !ImGui::GetIO().MouseDown[ImGuiMouseButton_Left]) {
            state->ClearText();
            isShowingVirtualKeyboard = true;
            SDL_StartTextInput();
        }
    } else if (isShowingVirtualKeyboard) {
        isShowingVirtualKeyboard = false;
        SDL_StopTextInput();
    }
}
#endif
