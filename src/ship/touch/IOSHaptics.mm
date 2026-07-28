#ifdef __IOS__
#import <UIKit/UIKit.h>
#import <CoreHaptics/CoreHaptics.h>

// Touch-play haptics, two layers:
//  - Taps: UIImpactFeedbackGenerator micro-impacts when a finger lands on an overlay button,
//    so glass feels like it has switches under it.
//  - Rumble: the game's N64 Rumble Pak line routed to the Taptic Engine. Majora's Mask drives
//    the pak PWM-style (rapid on/off at frame rate) to fake intensity levels, so the player
//    object is created ONCE and reused — start/stop must be cheap.
// CoreHaptics needs no entitlement, and every supported device (iPhone 8+) has the hardware;
// where it is absent (iPads) every call degrades to a silent no-op.

static UIImpactFeedbackGenerator* sTapLight = nil;
static UIImpactFeedbackGenerator* sTapMedium = nil;
static UIImpactFeedbackGenerator* sTapRigid = nil;

static CHHapticEngine* sEngine = nil;
static id<CHHapticPatternPlayer> sRumblePlayer = nil;
static float sRumbleIntensity = -1.0f; // player must be rebuilt when this changes
static bool sRumbleActive = false;

static bool EnsureEngine(void) {
    if (@available(iOS 13.0, *)) {
        if (!CHHapticEngine.capabilitiesForHardware.supportsHaptics) {
            return false;
        }
        if (sEngine == nil) {
            NSError* err = nil;
            sEngine = [[CHHapticEngine alloc] initAndReturnError:&err];
            if (err != nil || sEngine == nil) {
                sEngine = nil;
                return false;
            }
            sEngine.playsHapticsOnly = YES;
            // The engine dies whenever the app backgrounds or the media server resets; the
            // reset handler plus the unconditional start below bring it back on demand.
            __weak CHHapticEngine* weakEngine = sEngine;
            sEngine.resetHandler = ^{
                [weakEngine startAndReturnError:nil];
            };
        }
        // Cheap when already running; revives a stopped engine after backgrounding.
        return [sEngine startAndReturnError:nil];
    }
    return false;
}

extern "C" void IOSHapticsTap(int kind) {
    // 0 = light (buttons, pills), 1 = medium (stick spawn), 2 = rigid (ocarina notes).
    UIImpactFeedbackGenerator** gen = &sTapLight;
    UIImpactFeedbackStyle style = UIImpactFeedbackStyleLight;
    if (kind == 1) {
        gen = &sTapMedium;
        style = UIImpactFeedbackStyleMedium;
    } else if (kind == 2) {
        gen = &sTapRigid;
        style = UIImpactFeedbackStyleRigid;
    }
    if (*gen == nil) {
        *gen = [[UIImpactFeedbackGenerator alloc] initWithStyle:style];
    }
    [*gen impactOccurred];
    [*gen prepare]; // keep the Taptic Engine warm for the next press
}

extern "C" void IOSHapticsRumbleStart(float intensity) {
    if (@available(iOS 13.0, *)) {
        if (intensity <= 0.0f || !EnsureEngine()) {
            return;
        }
        intensity = intensity > 1.0f ? 1.0f : intensity;
        NSError* err = nil;
        if (sRumblePlayer == nil || sRumbleIntensity != intensity) {
            CHHapticEventParameter* pIntensity =
                [[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticIntensity
                                                              value:intensity];
            CHHapticEventParameter* pSharpness =
                [[CHHapticEventParameter alloc] initWithParameterID:CHHapticEventParameterIDHapticSharpness value:0.4f];
            // 30s continuous window: the pak line always stops long before this; Stop() cancels.
            CHHapticEvent* ev = [[CHHapticEvent alloc] initWithEventType:CHHapticEventTypeHapticContinuous
                                                              parameters:@[ pIntensity, pSharpness ]
                                                            relativeTime:0.0
                                                                duration:30.0];
            CHHapticPattern* pattern = [[CHHapticPattern alloc] initWithEvents:@[ ev ] parameters:@[] error:&err];
            if (err != nil || pattern == nil) {
                return;
            }
            sRumblePlayer = [sEngine createPlayerWithPattern:pattern error:&err];
            if (err != nil || sRumblePlayer == nil) {
                sRumblePlayer = nil;
                return;
            }
            sRumbleIntensity = intensity;
        }
        if (!sRumbleActive) {
            [sRumblePlayer startAtTime:CHHapticTimeImmediate error:&err];
            sRumbleActive = (err == nil);
        }
    }
}

extern "C" void IOSHapticsRumbleStop(void) {
    if (@available(iOS 13.0, *)) {
        if (sRumblePlayer != nil && sRumbleActive) {
            [sRumblePlayer stopAtTime:CHHapticTimeImmediate error:nil];
        }
        sRumbleActive = false;
    }
}
#endif
