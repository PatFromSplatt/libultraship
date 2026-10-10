#include "libultraship/controller/controldeck/ControlDeck.h"

#include "ship/Context.h"
#include "ship/touch/TouchControllerState.h"
#include "libultraship/controller/controldevice/controller/Controller.h"
#include "libultraship/controller/controldevice/controller/mapping/ControllerDefaultMappings.h"
#include "ship/utils/StringHelper.h"
#include <imgui.h>
#include <algorithm>
#include "ship/controller/controldevice/controller/mapping/mouse/WheelHandler.h"

namespace LUS {
ControlDeck::ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks,
                         std::shared_ptr<Ship::ControllerDefaultMappings> controllerDefaultMappings,
                         std::unordered_map<CONTROLLERBUTTONS_T, std::string> buttonNames)
    : Ship::ControlDeck(additionalBitmasks, controllerDefaultMappings, buttonNames), mPads(nullptr) {
    std::vector<CONTROLLERBUTTONS_T> bitmasks;
    for (auto [bitmask, name] : buttonNames) {
        bitmasks.push_back(bitmask);
    }
    bitmasks.insert(bitmasks.end(), additionalBitmasks.begin(), additionalBitmasks.end());
    for (int32_t i = 0; i < MAXCONTROLLERS; i++) {
        mPorts.push_back(std::make_shared<Ship::ControlPort>(i, std::make_shared<Controller>(i, bitmasks)));
    }
}

ControlDeck::ControlDeck(std::vector<CONTROLLERBUTTONS_T> additionalBitmasks)
    : ControlDeck(additionalBitmasks, std::make_shared<LUS::ControllerDefaultMappings>(),
                  std::unordered_map<CONTROLLERBUTTONS_T, std::string>({
                      { BTN_A, "A" },
                      { BTN_B, "B" },
                      { BTN_L, "L" },
                      { BTN_R, "R" },
                      { BTN_Z, "Z" },
                      { BTN_START, "Start" },
                      { BTN_CLEFT, "CLeft" },
                      { BTN_CRIGHT, "CRight" },
                      { BTN_CUP, "CUp" },
                      { BTN_CDOWN, "CDown" },
                      { BTN_DLEFT, "DLeft" },
                      { BTN_DRIGHT, "DRight" },
                      { BTN_DUP, "DUp" },
                      { BTN_DDOWN, "DDown" },
                  })) {
}

ControlDeck::ControlDeck() : ControlDeck(std::vector<CONTROLLERBUTTONS_T>()) {
}

OSContPad* ControlDeck::GetPads() {
    return mPads;
}

void ControlDeck::WriteToPad(void* pad) {
    WriteToOSContPad((OSContPad*)pad);
}

void ControlDeck::WriteToOSContPad(OSContPad* pad) {
    SDL_PumpEvents();
    Ship::WheelHandler::GetInstance()->Update();

    // Touch camera drag and button edges are consumed by every read, blocked or not, so input
    // gathered while blocked can never land later as one jump or a replayed press.
    auto& touch = Ship::TouchControllerState::Instance();
    const float touchCamX = touch.camAccumX.exchange(0.0f);
    const float touchCamY = touch.camAccumY.exchange(0.0f);
    const uint16_t touchHeld = touch.buttons.load();
    const uint16_t touchPressed = touch.pressedLatch.exchange(0);
    const uint16_t touchReleased = touch.releasedLatch.exchange(0);
    static uint16_t sLastTouchOut = 0;
    static uint16_t sLastTouchHeld = 0;
    static uint16_t sDeferredTouch = 0;

    if (AllGameInputBlocked()) {
        sLastTouchOut = 0;
        sLastTouchHeld = 0;
        sDeferredTouch = 0;
        return;
    }

    mPads = pad;

    for (size_t i = 0; i < mPorts.size(); i++) {
        const std::shared_ptr<Ship::Controller> controller = mPorts[i]->GetConnectedController();

        if (controller != nullptr) {
            controller->ReadToPad(&pad[i]);
        }
    }

    // Merge on-screen touch controls into port 1 (touch and a physical controller can both be
    // active; OR semantics match how multiple mappings already combine). A tap that went down and
    // up since the last read shows as pressed for this one read. A new press of a button the last
    // read reported (released since then, or already up at that read because it was a latched tap)
    // shows as released now and pressed on the next read, so it always gets its own rising edge.
    const uint16_t touchBounce = (uint16_t)(sLastTouchOut & touchPressed & (touchReleased | (uint16_t)~sLastTouchHeld));
    const uint16_t touchOut = (uint16_t)((touchHeld | touchPressed | sDeferredTouch) & ~touchBounce);
    sDeferredTouch = touchBounce;
    sLastTouchOut = touchOut;
    sLastTouchHeld = touchHeld;
    pad[0].button |= touchOut;
    if (pad[0].stick_x == 0 && pad[0].stick_y == 0) {
        pad[0].stick_x = touch.stickX.load();
        pad[0].stick_y = touch.stickY.load();
    }
    if (touchCamX != 0.0f || touchCamY != 0.0f) {
        pad[0].right_stick_x = (int8_t)(std::clamp(touchCamX, -1.0f, 1.0f) * 127.0f);
        pad[0].right_stick_y = (int8_t)(std::clamp(touchCamY, -1.0f, 1.0f) * 127.0f);
    }
    // Gyro is a RATE, matching SDLGyroMapping::UpdatePad — assign it, never accumulate, or the
    // aim drifts away permanently. If the sensor stops delivering samples (disabled, or the app
    // was backgrounded) the last rate must not stay applied, so track the sample counter.
    static uint32_t sLastGyroSeq = 0;
    const uint32_t gyroSeq = touch.gyroSeq.load();
    if (gyroSeq != sLastGyroSeq) {
        sLastGyroSeq = gyroSeq;
        pad[0].gyro_x = touch.gyroX.load();
        pad[0].gyro_y = touch.gyroY.load();
    }
}
} // namespace LUS
