#include "ship/touch/TouchControlOverlay.h"
#include "ship/touch/TouchControllerState.h"
#include "ship/Context.h"
#include "ship/config/ConsoleVariable.h"
#include "ship/window/Window.h"
#include "ship/window/gui/Gui.h"
#include "libultraship/libultra/controller.h"
#include <cmath>
#include <algorithm>

namespace Ship {

// Majora's Mask HUD palette (alpha applied per gTouch.Opacity at draw time)
static constexpr ImU32 kColA = IM_COL32(90, 120, 255, 255);      // MM A button blue
static constexpr ImU32 kColB = IM_COL32(80, 190, 90, 255);       // MM B button green
static constexpr ImU32 kColC = IM_COL32(255, 205, 60, 255);      // MM C button yellow
static constexpr ImU32 kColNeutral = IM_COL32(205, 205, 205, 255);

#if defined(__IOS__) || defined(__ANDROID__)
static constexpr int32_t kEnabledDefault = 1;
#else
static constexpr int32_t kEnabledDefault = 0;
#endif

#ifdef __IOS__
extern "C" void GetIOSSafeAreaInsets(float* top, float* left, float* bottom, float* right);
extern "C" void IOSGyroSetEnabled(bool enabled, float sensitivity);
#endif

TouchControlOverlay& TouchControlOverlay::Instance() {
    static TouchControlOverlay sInstance;
    return sInstance;
}

bool TouchControlOverlay::Enabled() {
    auto ctx = Context::GetInstance();
    if (ctx == nullptr || ctx->GetConsoleVariables() == nullptr) {
        return false;
    }
    return ctx->GetConsoleVariables()->GetInteger("gTouch.Enabled", kEnabledDefault) != 0;
}

void TouchControlOverlay::RebuildLayout(ImVec2 displaySize) {
    mDisplaySize = displaySize;
#ifdef __IOS__
    GetIOSSafeAreaInsets(&mSafeTop, &mSafeLeft, &mSafeBottom, &mSafeRight);
#endif
    const float W = displaySize.x;
    const float H = displaySize.y;
    const float scale = Context::GetInstance()->GetConsoleVariables()->GetFloat("gTouch.Scale", 1.0f);
    const float sl = mSafeLeft, sr = mSafeRight, st = mSafeTop;

    mElements.clear();
    // The floating-stick zone is handled in HandleFingerEvent (any touch in the left
    // region not hitting a button becomes the stick); Stick element only carries draw
    // geometry defaults.
    mStickRadius = 0.11f * H * scale;

    auto pill = [&](TouchElementId id, float cx, float cy, float hw, float hh, uint16_t mask, const char* label) {
        mElements.push_back({ id, ImVec2(cx, cy), 0.0f, ImVec2(hw * scale, hh * scale), mask, label });
    };
    auto circle = [&](TouchElementId id, float cx, float cy, float r, uint16_t mask, const char* label) {
        mElements.push_back({ id, ImVec2(cx, cy), r * scale, ImVec2(), mask, label });
    };

    if (!mOcarinaLayout) {
        // hug the right edge/corner: thumbs rest at the phone's rim, not its middle
        circle(TouchElementId::A, 0.940f * W - sr, 0.76f * H, 0.085f * H, BTN_A, "A");
        circle(TouchElementId::B, 0.848f * W - sr, 0.875f * H, 0.065f * H, BTN_B, "B");
        const float cX = 0.918f * W - sr;
        const float cY = 0.42f * H;
        const float cSp = 0.080f * H * scale;
        circle(TouchElementId::CUp, cX, cY - cSp, 0.042f * H, BTN_CUP, "C");
        circle(TouchElementId::CDown, cX, cY + cSp, 0.042f * H, BTN_CDOWN, "C");
        circle(TouchElementId::CLeft, cX - cSp, cY, 0.042f * H, BTN_CLEFT, "C");
        circle(TouchElementId::CRight, cX + cSp, cY, 0.042f * H, BTN_CRIGHT, "C");
    } else {
        // Piano keys, low D to high C-up, matching MM staff order.
        const float keyY = 0.80f * H;
        const float hw = 0.048f * W;
        const float hh = 0.10f * H;
        float x = 0.52f * W;
        const float step = 0.105f * W;
        pill(TouchElementId::NoteD, x, keyY, hw, hh, BTN_A, "D");
        pill(TouchElementId::NoteCDown, x += step, keyY, hw, hh, BTN_CDOWN, "v");
        pill(TouchElementId::NoteCRight, x += step, keyY, hw, hh, BTN_CRIGHT, ">");
        pill(TouchElementId::NoteCLeft, x += step, keyY, hw, hh, BTN_CLEFT, "<");
        pill(TouchElementId::NoteCUp, x += step, keyY, hw, hh, BTN_CUP, "^");
    }

    pill(TouchElementId::Z, 0.070f * W + sl, 0.085f * H + st, 0.052f * W, 0.042f * H, BTN_Z, "Z");
    pill(TouchElementId::L, 0.158f * W + sl, 0.085f * H + st, 0.026f * W, 0.032f * H, BTN_L, "L");
    pill(TouchElementId::R, 0.930f * W - sr, 0.085f * H + st, 0.052f * W, 0.042f * H, BTN_R, "R");

    // system pills: small, tucked against the top edge, rarely touched
    const float pillY = 0.050f * H + st;
    pill(TouchElementId::Start, 0.42f * W, pillY, 0.024f * W, 0.027f * H, BTN_START, "start");
    pill(TouchElementId::Gear, 0.48f * W, pillY, 0.024f * W, 0.027f * H, 0, "menu");
    pill(TouchElementId::Ocarina, 0.54f * W, pillY, 0.024f * W, 0.027f * H, 0, "song");
    pill(TouchElementId::Eye, 0.60f * W, pillY, 0.024f * W, 0.027f * H, 0, "hide");
}

TouchElement* TouchControlOverlay::HitTest(ImVec2 px) {
    for (auto& el : mElements) {
        if (el.radius > 0.0f) {
            const float dx = px.x - el.center.x;
            const float dy = px.y - el.center.y;
            // generous touch slop: 1.35x visual radius
            if (dx * dx + dy * dy <= (el.radius * 1.35f) * (el.radius * 1.35f)) {
                return &el;
            }
        } else {
            if (std::fabs(px.x - el.center.x) <= el.halfSize.x * 1.25f &&
                std::fabs(px.y - el.center.y) <= el.halfSize.y * 1.35f) {
                return &el;
            }
        }
    }
    return nullptr;
}

void TouchControlOverlay::UpdateStick(ImVec2 px) {
    float dx = px.x - mStickBase.x;
    float dy = px.y - mStickBase.y;
    const float len = std::sqrt(dx * dx + dy * dy);
    if (len > mStickRadius && len > 0.0f) {
        dx *= mStickRadius / len;
        dy *= mStickRadius / len;
    }
    mStickPos = ImVec2(mStickBase.x + dx, mStickBase.y + dy);
    float nx = dx / mStickRadius; // -1..1
    float ny = -dy / mStickRadius; // screen y down -> stick y up
    const float deadZone = 0.10f;
    auto shape = [&](float v) -> float {
        if (std::fabs(v) < deadZone) {
            return 0.0f;
        }
        return (v - std::copysign(deadZone, v)) / (1.0f - deadZone);
    };
    nx = shape(nx);
    ny = shape(ny);
    auto& state = TouchControllerState::Instance();
    state.stickX.store((int8_t)std::clamp(nx * 127.0f, -128.0f, 127.0f));
    state.stickY.store((int8_t)std::clamp(ny * 127.0f, -128.0f, 127.0f));
}

void TouchControlOverlay::ComposeButtons() {
    uint16_t mask = 0;
    for (const auto& [finger, id] : mFingerOwner) {
        for (const auto& el : mElements) {
            if (el.id == id) {
                mask |= el.buttonMask;
                break;
            }
        }
    }
    TouchControllerState::Instance().buttons.store(mask);
}

void TouchControlOverlay::HandlePillRelease(TouchElementId id) {
    auto gui = Context::GetInstance()->GetWindow()->GetGui();
    switch (id) {
        case TouchElementId::Gear:
            // Prefer the full settings menu (what Escape opens on desktop); fall back to
            // the classic menubar when no menu window is registered.
            if (gui != nullptr && gui->GetMenu() != nullptr) {
                gui->GetMenu()->ToggleVisibility();
            } else if (gui != nullptr && gui->GetMenuBar() != nullptr) {
                gui->GetMenuBar()->ToggleVisibility();
            }
            break;
        case TouchElementId::Ocarina:
            mOcarinaLayout = !mOcarinaLayout;
            RebuildLayout(mDisplaySize);
            break;
        case TouchElementId::Eye:
            mHidden = !mHidden;
            break;
        default:
            break;
    }
}

void TouchControlOverlay::SetPhysicalControllerConnected(bool connected) {
    mAutoHidden = connected;
}

void TouchControlOverlay::HandleFingerEvent(const SDL_TouchFingerEvent& finger, uint32_t type) {
    if (!Enabled() || mDisplaySize.x <= 0.0f) {
        return;
    }
    const ImVec2 px(finger.x * mDisplaySize.x, finger.y * mDisplaySize.y);
    auto gui = Context::GetInstance()->GetWindow()->GetGui();
    const bool menuOpen = gui != nullptr && gui->GetMenuOrMenubarVisible();

    if (mHidden || mAutoHidden) {
        // only the ghost dot (bottom-right corner) unhides
        if (type == SDL_FINGERUP && px.x > mDisplaySize.x * 0.94f && px.y > mDisplaySize.y * 0.88f) {
            mHidden = false;
            mAutoHidden = false;
        }
        return;
    }

    switch (type) {
        case SDL_FINGERDOWN: {
            TouchElement* el = HitTest(px);
            if (menuOpen) {
                // menu drives via touch-as-mouse; only the gear pill stays live
                if (el != nullptr && el->id == TouchElementId::Gear) {
                    mFingerOwner[finger.fingerId] = el->id;
                }
                return;
            }
            if (el != nullptr) {
                mFingerOwner[finger.fingerId] = el->id;
                ComposeButtons();
            } else if (px.x < mDisplaySize.x * 0.40f && px.y > mDisplaySize.y * 0.30f && !mStickActive) {
                // floating stick: base spawns where the finger lands in the left zone
                mStickActive = true;
                mFingerOwner[finger.fingerId] = TouchElementId::Stick;
                const bool fixed =
                    Context::GetInstance()->GetConsoleVariables()->GetInteger("gTouch.FixedStick", 0) != 0;
                mStickBase = fixed ? ImVec2(mDisplaySize.x * 0.17f, mDisplaySize.y * 0.70f) : px;
                mStickPos = mStickBase;
                UpdateStick(px);
            } else if (mCameraFinger == -1) {
                mCameraFinger = finger.fingerId;
                mFingerOwner[finger.fingerId] = TouchElementId::CameraZone;
                mLastCameraPx = px;
            }
            break;
        }
        case SDL_FINGERMOTION: {
            auto it = mFingerOwner.find(finger.fingerId);
            if (it == mFingerOwner.end()) {
                return;
            }
            if (it->second == TouchElementId::Stick) {
                UpdateStick(px);
            } else if (it->second == TouchElementId::CameraZone) {
                const float sens =
                    Context::GetInstance()->GetConsoleVariables()->GetFloat("gTouch.CameraSensitivity", 1.0f);
                mCamAccumX += (px.x - mLastCameraPx.x) / mDisplaySize.x * 6.0f * sens;
                mCamAccumY += (px.y - mLastCameraPx.y) / mDisplaySize.y * 6.0f * sens;
                mLastCameraPx = px;
            }
            break;
        }
        case SDL_FINGERUP: {
            auto it = mFingerOwner.find(finger.fingerId);
            if (it == mFingerOwner.end()) {
                return;
            }
            const TouchElementId id = it->second;
            mFingerOwner.erase(it);
            if (id == TouchElementId::Stick) {
                mStickActive = false;
                auto& state = TouchControllerState::Instance();
                state.stickX.store(0);
                state.stickY.store(0);
            } else if (id == TouchElementId::CameraZone) {
                mCameraFinger = -1;
            } else {
                HandlePillRelease(id);
            }
            ComposeButtons();
            break;
        }
        default:
            break;
    }
}

void TouchControlOverlay::DrawElement(const TouchElement& el, bool pressed, ImDrawList* dl, float opacity) {
    ImU32 col = kColNeutral;
    switch (el.id) {
        case TouchElementId::A:
        case TouchElementId::NoteD:
            col = kColA;
            break;
        case TouchElementId::B:
            col = kColB;
            break;
        case TouchElementId::CUp:
        case TouchElementId::CDown:
        case TouchElementId::CLeft:
        case TouchElementId::CRight:
        case TouchElementId::NoteCUp:
        case TouchElementId::NoteCDown:
        case TouchElementId::NoteCLeft:
        case TouchElementId::NoteCRight:
            col = kColC;
            break;
        default:
            break;
    }
    float a = opacity * (pressed ? 1.4f : 1.0f);
    a = std::min(a, 1.0f);
    const ImU32 fill = (col & 0x00FFFFFF) | ((ImU32)(a * 255.0f) << 24);
    const ImU32 line = (col & 0x00FFFFFF) | ((ImU32)(std::min(a * 1.6f, 1.0f) * 255.0f) << 24);
    const float grow = pressed ? 1.06f : 1.0f;

    if (el.radius > 0.0f) {
        dl->AddCircleFilled(el.center, el.radius * grow, fill);
        dl->AddCircle(el.center, el.radius * grow, line, 0, 2.0f);
    } else {
        const ImVec2 mn(el.center.x - el.halfSize.x * grow, el.center.y - el.halfSize.y * grow);
        const ImVec2 mx(el.center.x + el.halfSize.x * grow, el.center.y + el.halfSize.y * grow);
        dl->AddRectFilled(mn, mx, fill, el.halfSize.y * 0.8f);
        dl->AddRect(mn, mx, line, el.halfSize.y * 0.8f, 0, 2.0f);
    }

    // C-button directional triangles
    auto tri = [&](float ox, float oy, float rot) {
        const float s = (el.radius > 0.0f ? el.radius : el.halfSize.y) * 0.45f;
        const float cx = el.center.x + ox;
        const float cy = el.center.y + oy;
        const float c = std::cos(rot), sn = std::sin(rot);
        auto pt = [&](float x, float y) { return ImVec2(cx + x * c - y * sn, cy + x * sn + y * c); };
        dl->AddTriangleFilled(pt(0, -s), pt(s * 0.85f, s * 0.6f), pt(-s * 0.85f, s * 0.6f),
                              IM_COL32(60, 45, 0, (int)(a * 255)));
    };
    switch (el.id) {
        case TouchElementId::CUp:
        case TouchElementId::NoteCUp:
            tri(0, 0, 0.0f);
            break;
        case TouchElementId::CDown:
        case TouchElementId::NoteCDown:
            tri(0, 0, 3.14159f);
            break;
        case TouchElementId::CLeft:
        case TouchElementId::NoteCLeft:
            tri(0, 0, -1.5708f);
            break;
        case TouchElementId::CRight:
        case TouchElementId::NoteCRight:
            tri(0, 0, 1.5708f);
            break;
        default: {
            if (el.label[0] != '\0' && el.id != TouchElementId::CUp) {
                const ImVec2 ts = ImGui::CalcTextSize(el.label);
                dl->AddText(ImVec2(el.center.x - ts.x * 0.5f, el.center.y - ts.y * 0.5f),
                            IM_COL32(255, 255, 255, (int)(std::min(a * 1.8f, 1.0f) * 255)), el.label);
            }
            break;
        }
    }
}

void TouchControlOverlay::Draw() {
    if (!Enabled()) {
        return;
    }
    auto& state = TouchControllerState::Instance();

#ifdef __IOS__
    // keep the gyro shim in sync with its CVars (start/stop is idempotent)
    static bool sGyroWasEnabled = false;
    auto cvars = Context::GetInstance()->GetConsoleVariables();
    const bool gyroEnabled = cvars->GetInteger("gTouch.GyroEnabled", 0) != 0;
    if (gyroEnabled != sGyroWasEnabled) {
        IOSGyroSetEnabled(gyroEnabled, cvars->GetFloat("gTouch.GyroSensitivity", 1.0f));
        sGyroWasEnabled = gyroEnabled;
    }
#endif

    // drain camera accumulators into per-frame deltas
    state.cameraX.store(std::clamp(mCamAccumX, -1.0f, 1.0f));
    state.cameraY.store(std::clamp(mCamAccumY, -1.0f, 1.0f));
    mCamAccumX = 0.0f;
    mCamAccumY = 0.0f;

    ImDrawList* dl = ImGui::GetForegroundDrawList();
    const ImVec2 displaySize = ImGui::GetIO().DisplaySize;

    if (mHidden || mAutoHidden) {
        state.buttons.store(0);
        // ghost dot to unhide
        dl->AddCircleFilled(ImVec2(displaySize.x * 0.97f, displaySize.y * 0.94f), displaySize.y * 0.018f,
                            IM_COL32(255, 255, 255, 40));
        return;
    }

    if (displaySize.x != mDisplaySize.x || displaySize.y != mDisplaySize.y || mElements.empty()) {
        RebuildLayout(displaySize);
    }

    const float opacity = Context::GetInstance()->GetConsoleVariables()->GetFloat("gTouch.Opacity", 0.35f);

    // stick: zone hint ring when idle, base+knob when active
    if (mStickActive) {
        dl->AddCircle(mStickBase, mStickRadius, IM_COL32(255, 255, 255, (int)(opacity * 200)), 0, 2.5f);
        dl->AddCircleFilled(mStickPos, mStickRadius * 0.42f, IM_COL32(255, 255, 255, (int)(opacity * 230)));
    } else {
        const ImVec2 hint(mDisplaySize.x * 0.135f, mDisplaySize.y * 0.76f);
        dl->AddCircle(hint, mStickRadius * 0.6f, IM_COL32(255, 255, 255, (int)(opacity * 90)), 0, 1.5f);
        dl->AddCircleFilled(hint, mStickRadius * 0.12f, IM_COL32(255, 255, 255, (int)(opacity * 90)));
    }

    for (const auto& el : mElements) {
        bool pressed = false;
        for (const auto& [finger, id] : mFingerOwner) {
            if (id == el.id) {
                pressed = true;
                break;
            }
        }
        if (el.id == TouchElementId::Ocarina && mOcarinaLayout) {
            pressed = true; // stays lit while the piano layout is active
        }
        DrawElement(el, pressed, dl, opacity);
    }
}

} // namespace Ship
