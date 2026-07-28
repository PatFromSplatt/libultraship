#pragma once
#include <SDL2/SDL_events.h>
#include <unordered_map>
#include <vector>
#include <cstdint>
#include "imgui.h"

namespace Ship {

enum class TouchElementId {
    Stick,
    A,
    B,
    CUp,
    CDown,
    CLeft,
    CRight,
    Z,
    L,
    R,
    Start,
    Gear,
    Ocarina,
    Eye,
    CameraZone, // implicit: any unclaimed touch area
    NoteD,
    NoteCDown,
    NoteCRight,
    NoteCLeft,
    NoteCUp,
};

struct TouchElement {
    TouchElementId id;
    ImVec2 center{};
    float radius = 0.0f; // > 0: circular hit test / draw
    ImVec2 halfSize{};   // radius == 0: rounded-rect pill / key
    uint16_t buttonMask = 0;
    const char* label = "";
};

// On-screen touch controls. Consumes raw SDL finger events (multi-touch chords work,
// unlike ImGui's single-pointer touch-as-mouse) and writes TouchControllerState, which
// ControlDeck merges into the N64 pad. Draws via the ImGui foreground draw list from
// Gui::DrawGame(). Inert unless gTouch.Enabled (defaults on only for mobile builds).
class TouchControlOverlay {
  public:
    static TouchControlOverlay& Instance();

    void HandleFingerEvent(const SDL_TouchFingerEvent& finger, uint32_t type);
    void Draw();             // call once per frame from Gui::DrawGame
    void ApplyTouchScroll(); // call once per frame from Gui::StartFrame, right after NewFrame()
    void SetPhysicalControllerConnected(bool connected);
    bool IsPhysicalControllerConnected() const {
        return mPhysicalControllerConnected;
    }
    bool IsOcarinaLayout() const {
        return mOcarinaLayout;
    }

  private:
    bool Enabled();
    void RebuildLayout(ImVec2 displaySize);
    TouchElement* HitTest(ImVec2 px);
    void UpdateStick(ImVec2 px);
    void ComposeButtons();
    void HandlePillRelease(TouchElementId id);
    void DrawElement(const TouchElement& el, bool pressed, ImDrawList* dl, float opacity);

    std::vector<TouchElement> mElements;
    std::unordered_map<SDL_FingerID, TouchElementId> mFingerOwner;

    bool mStickActive = false;
    ImVec2 mStickBase{};
    ImVec2 mStickPos{};
    float mStickRadius = 0.0f;

    SDL_FingerID mCameraFinger = -1;

    // Menu drag-to-scroll. iOS never emits SDL_MOUSEWHEEL and ImGui has no touch scrolling, so
    // a tracked finger drag is the menu's only scroll input.
    bool HandleMenuScrollFinger(ImVec2 px, SDL_FingerID id, uint32_t type);
    SDL_FingerID mScrollFinger = -1;
    ImVec2 mScrollStart{};
    ImVec2 mScrollLast{};
    float mScrollAccumX = 0.0f;
    float mScrollAccumY = 0.0f;
    bool mScrollPastSlop = false;
    bool mScrollRejected = false;
    unsigned int mScrollWindowId = 0;
    ImVec2 mLastCameraPx{};
    float mCamAccumX = 0.0f;
    float mCamAccumY = 0.0f;

    bool mOcarinaLayout = false;
    bool mHidden = false;
    bool mAutoHidden = false;                  // overlay hidden because a controller attached
    bool mPhysicalControllerConnected = false; // tracked separately: rumble routing needs the
                                               // raw fact even if hiding behaviour changes
    ImVec2 mDisplaySize{};
    float mSafeLeft = 0.0f, mSafeRight = 0.0f, mSafeTop = 0.0f, mSafeBottom = 0.0f;
};

} // namespace Ship
