# libultraship — iOS support (`ios` branch)

This branch adds an **iOS platform layer** to libultraship, the engine behind the
HarbourMasters ports. It is what makes [2 Ship 2 Harkinian on
iPhone](https://github.com/PatFromSplatt/2ship2harkinian/tree/ios) possible, and is written
to be reusable by any other game built on this engine.

It is an unofficial community branch. Nothing here is endorsed by HarbourMasters or Kenix3,
and it contains no game assets of any kind.

## What this branch adds

| Area | What it does |
| --- | --- |
| **Build** | iOS CMake target (device, arm64, Metal-only), app-bundle packaging, `PLATFORM=OS64` override so Ninja builds work without an Xcode project |
| **Boot** | Keeps SDL2main's UIKit bootstrap intact, fails loudly instead of silently when a renderer can't be created, and survives a config naming a backend the build doesn't contain |
| **Touch controls** | Full on-screen controller: floating analog stick, N64-styled A/B and C-buttons, Z/L/R, drag-to-look camera, a dedicated ocarina layout, auto-hide when a physical controller connects. True multi-touch — chords like run + attack work |
| **Gyro** | CoreMotion device tilt feeds the pad's native gyro fields (opt-in) |
| **Display** | Renders the 3D scene at the device's real pixel resolution while the UI stays in points; safe-area insets published once so every window clears the Dynamic Island and home indicator |
| **Frame pacing** | Drops interpolated frames when the device can't keep up, so the game runs at the correct *speed* rather than in slow motion |
| **UI** | One coherent scale model built around the 44pt minimum touch target, fonts rasterized at device density (crisp rather than upscaled), windows clamped so they can't open off-screen |
| **Lifecycle** | Correct background/resume behavior (no more ~1fps after returning to the app), screen kept awake, audio that ignores the silent switch |

Everything above is either compiled out or algebraically neutral on desktop platforms —
Windows, Linux and macOS builds are unaffected.

## Using it in your own port

1. Point your game's `libultraship` submodule at this fork's `ios` branch.
2. Configure with the iOS toolchain:

```bash
cmake -H. -Bbuild-cmake -GNinja \
  -DCMAKE_SYSTEM_NAME=iOS -DCMAKE_OSX_SYSROOT=iphoneos \
  -DCMAKE_OSX_ARCHITECTURES=arm64 -DCMAKE_OSX_DEPLOYMENT_TARGET=16.0 \
  -DPLATFORM=OS64 -DDEPLOYMENT_TARGET=16.0 \
  -DCMAKE_BUILD_TYPE=Release
```

3. On the game side you will need: an `Info.plist` declaring a landscape app with file
   sharing enabled, an app-bundle CMake target, and — importantly — **do not** `#define
   SDL_main main` on iOS, or SDL2main's UIKit bootstrap never runs and Metal has no app to
   attach to.

See [`2ship2harkinian`'s iOS branch](https://github.com/PatFromSplatt/2ship2harkinian/tree/ios)
for a complete worked example of the game-side half.

## Settings this branch adds

All are ordinary CVars, adjustable in-game:

| CVar | Default | Meaning |
| --- | --- | --- |
| `gTouch.Opacity` / `gTouch.Scale` | 0.35 / 1.0 | Look of the on-screen controls |
| `gTouch.FixedStick` | off | Anchor the stick instead of it appearing under your thumb |
| `gTouch.EdgeLayout` | off | Move buttons out to the screen edges |
| `gTouch.CameraSensitivity` | 1.0 | Drag-to-look speed |
| `gTouch.GyroEnabled` / `gTouch.GyroSensitivity` | off / 1.0 | Tilt aiming |
| `gSettings.NativeResolution` | on | Render the 3D scene at full device resolution |
| `gSettings.FrameDropCatchUp` | on | Keep wall-clock game speed when frames are missed |
| `gSettings.UIScale` | 1.0 | Menu chrome size |
| `gSettings.CrispFonts` | on | Rasterize fonts at device density (restart to apply) |

## Credits

Built on [libultraship](https://github.com/Kenix3/libultraship) by Kenix3 and contributors.
The touch-control layout takes inspiration from the
[Android ports](https://github.com/Waterdish/Shipwright-Android) by Waterdish.
