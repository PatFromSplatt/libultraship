# libultraship: iOS support (`ios-unified` branch)

This branch adds an iOS platform layer to libultraship. One engine branch serves both iOS ports:

| Game | Repository | Branch |
| --- | --- | --- |
| 2 Ship 2 Harkinian (Majora's Mask) | [PatFromSplatt/2ship2harkinian](https://github.com/PatFromSplatt/2ship2harkinian/tree/ios) | `ios` |
| Ship of Harkinian (Ocarina of Time) | [PatFromSplatt/Shipwright](https://github.com/PatFromSplatt/Shipwright/tree/ios) | `ios` |

Each game's `libultraship` submodule is meant to point at a commit on this fork's `ios-unified`
branch, replacing the earlier per-game engine branches (`ios` for 2Ship, `ios-lus-c57da1b4` for
SoH); check the game's `.gitmodules` and submodule pointer for what it currently uses. The base is
upstream [Kenix3/libultraship](https://github.com/Kenix3/libultraship) `port-maintenance` at
`7f9b86a5`.

This is an unofficial community branch. It is not endorsed by HarbourMasters or Kenix3 and
contains no game assets.

## iOS layer

| File | Purpose |
| --- | --- |
| `src/ship/touch/TouchControlOverlay.cpp`, `include/ship/touch/TouchControlOverlay.h` | On-screen controller drawn with ImGui from raw SDL finger events (true multi-touch): floating stick, A/B/C/Z/L/R/Start, D-pad, drag-to-look camera, ocarina note layout; hides while a physical controller is connected. |
| `include/ship/touch/TouchControllerState.h` | Atomic pad state written by the overlay and gyro; `LUS::ControlDeck::WriteToOSContPad` merges it into port 1. |
| `src/ship/touch/IOSGyro.mm` | CoreMotion rotation rate into the pad's gyro fields, sign-corrected for either landscape orientation. |
| `src/ship/touch/IOSHaptics.mm` | Tap feedback on overlay buttons (UIKit impact generators) and the Rumble Pak routed to the Taptic Engine (CoreHaptics) during touch play. |
| `src/ship/utils/IOSSafeArea.mm` | Window safe-area insets in UIKit points, so the GUI and overlay clear the notch/Dynamic Island and home indicator. |
| `src/ship/utils/IOSCertInfo.mm` | Reads `ExpirationDate` from `embedded.mobileprovision` so a game can show when a sideloaded build stops launching; 0 when there is no profile. |
| `src/ship/utils/IOSAudioSession.mm` | Re-activates the `AVAudioSession` on every return to the foreground, so a call, Siri or an alarm cannot leave the game silent. |
| `src/ship/utils/IOSBackgroundTask.mm` | Wraps `beginBackgroundTaskWithExpirationHandler:` so the background save gets time to finish before iOS suspends the app. |
| `include/ship/port/mobile/MobileImpl.h`, `src/ship/port/mobile/MobileImpl.cpp` | App lifecycle state (`SetAppBackgrounded` / `IsAppBackgrounded`), the once-per-backgrounding save request each game's autosave polls (`ConsumeBackgroundSaveRequest`), and `SetBackgroundSaveHandler`: the game's save routine, run once per background entry from the Metal park in `gfx_metal.cpp`, inside a background task. |

Hooks into existing files:

- `src/fast/backends/gfx_sdl2.cpp`: iOS SDL hints (screen stays awake, deferred home-indicator
  gesture, audio plays with the silent switch on), finger events to the overlay, and an SDL event
  watch that feeds the app lifecycle into `Ship::Mobile`.
- `src/fast/backends/gfx_metal.cpp`: real drawable scale for the Metal view, and no drawable is
  acquired while backgrounded (iOS drops background presents and the drawable pool starves).
- `ControlDeck.cpp`, `ControllerRumble.cpp`, `SDLAddRemoveDeviceEventHandler.cpp`: touch input
  merge, phone rumble, overlay hide/show on controller connect.
- `src/fast/Fast3dGui.cpp`, `src/ship/window/gui/*`: native-resolution rendering, UI scale, fonts
  rasterized at device density, safe-area work area, windows kept on screen.

Native iOS code is guarded by `__IOS__` (or `__IOS__ || __ANDROID__` where the mobile code
already was). The touch overlay compiles on every platform but is inert unless `gTouch.Enabled`.

## Building

```bash
cmake -H. -Bbuild-ios -GNinja \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0 \
  -DPLATFORM=OS64 -DDEPLOYMENT_TARGET=16.0 \
  -DCMAKE_BUILD_TYPE=Release -DENABLE_SCRIPTING=OFF -DDISABLE_DLL_LOADER=ON
cmake --build build-ios --target libultraship
```

The game side needs an `Info.plist` for a landscape app with file sharing enabled and an
app-bundle CMake target. It must not `#define SDL_main main` on iOS, or SDL2main's UIKit
bootstrap never runs and Metal has no app to attach to. See either game's `ios` branch.

## CI and the SDK pin

`.github/workflows/ios.yml` compiles `libultraship` standalone (iphoneos arm64, Metal-only,
deployment target 16.0) on pushes and pull requests to `ios-unified`. It runs on `macos-26` with
`DEVELOPER_DIR=/Applications/Xcode_26.6.app/Contents/Developer`, and a `Verify toolchain` step
fails the job unless `xcrun --sdk iphoneos --show-sdk-version` is `26.5`. Both game workflows use
the same pin.

Why the iOS 26.5 SDK and not 27: apps built with the iOS 27 SDK must adopt the UIScene lifecycle
or they fail to launch. SDL2 2.32.10's UIKit backend uses the app-delegate lifecycle and has no
scene support, so a 27-SDK build would crash at launch. The requirement is tied to the SDK the
app is built with, not the OS it runs on: 26.5-SDK builds run fine on iOS 27. Moving to a newer
SDK needs UIScene support in the SDL layer first.

## Credits

Built on [libultraship](https://github.com/Kenix3/libultraship) by Kenix3 and contributors. The
touch-control layout takes inspiration from the
[Android ports](https://github.com/Waterdish/Shipwright-Android) by Waterdish.
