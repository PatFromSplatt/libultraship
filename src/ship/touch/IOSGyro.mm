#ifdef __IOS__
#import <CoreMotion/CoreMotion.h>
#import <UIKit/UIKit.h>
#include <atomic>
#include "ship/touch/TouchControllerState.h"

static CMMotionManager* sMotion = nil;
static NSOperationQueue* sMotionQueue = nil;
static std::atomic<float> sSensitivity{ 1.0f };
static std::atomic<float> sOrientationSign{ 1.0f };

// UIKit accessors are main-thread only. Called once per frame from TouchControlOverlay::Draw
// while gyro is enabled; the sensor thread only reads the cached sign.
extern "C" void IOSGyroRefreshOrientation(void) {
    UIInterfaceOrientation o = UIInterfaceOrientationUnknown;
    for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
        if ([scene isKindOfClass:[UIWindowScene class]]) {
            o = ((UIWindowScene*)scene).interfaceOrientation;
            break;
        }
    }
    // Both landscapes are allowed; a 180-degree in-plane flip inverts the device X and Y axes
    // relative to the screen, so without this gyro is correct one way up and inverted the other.
    sOrientationSign.store(o == UIInterfaceOrientationLandscapeLeft ? -1.0f : 1.0f, std::memory_order_relaxed);
}

extern "C" void IOSGyroSetSensitivity(float sensitivity) {
    sSensitivity.store(sensitivity, std::memory_order_relaxed);
}

// Feeds device rotation rate into the pad's native gyro fields. The magnitude convention matches
// SDLGyroMapping::UpdatePad exactly: raw rad/s times the user's sensitivity — which is what
// z_player.c's `* 720` is tuned against. (The previous 0.02 factor made it ~50x too weak to
// perceive.)
extern "C" void IOSGyroSetEnabled(bool enabled, float sensitivity) {
    sSensitivity.store(sensitivity, std::memory_order_relaxed);
    if (enabled) {
        if (sMotion == nil) {
            sMotion = [[CMMotionManager alloc] init];
        }
        if (!sMotion.deviceMotionAvailable || sMotion.deviceMotionActive) {
            return; // already running; the block picks up the new sensitivity next sample
        }
        IOSGyroRefreshOrientation();
        if (sMotionQueue == nil) {
            // NOT mainQueue: SDL_main never returns to the run loop on iOS, so main-queue work
            // only drains inside UIKit's brief event pump. TouchControllerState is all atomics,
            // so there is no reason to serialise onto the main thread.
            sMotionQueue = [[NSOperationQueue alloc] init];
            sMotionQueue.name = @"gyro";
            sMotionQueue.maxConcurrentOperationCount = 1;
            sMotionQueue.qualityOfService = NSQualityOfServiceUserInteractive;
        }
        // deviceMotion (not raw gyro) so the rate is bias-corrected: raw CMGyroData carries a
        // bias that reads as constant, un-recentring camera drift.
        sMotion.deviceMotionUpdateInterval = 1.0 / 60.0;
        [sMotion startDeviceMotionUpdatesToQueue:sMotionQueue
                                     withHandler:^(CMDeviceMotion* motion, NSError* error) {
                                       if (motion == nil) {
                                           return;
                                       }
                                       auto& state = Ship::TouchControllerState::Instance();
                                       const float s = sSensitivity.load(std::memory_order_relaxed) *
                                                       sOrientationSign.load(std::memory_order_relaxed);
                                       // CoreMotion axes are bolted to the hardware: +X = portrait
                                       // short edge, +Y = portrait long edge, +Z out of the screen.
                                       // Held in landscape, rotation about X is YAW and about Y is
                                       // PITCH; Z is roll and is deliberately unused. The game reads
                                       // pad.gyro_x for vertical aim and pad.gyro_y for horizontal.
                                       state.gyroX.store((float)motion.rotationRate.y * s, std::memory_order_relaxed);
                                       state.gyroY.store((float)motion.rotationRate.x * s, std::memory_order_relaxed);
                                       state.gyroSeq.fetch_add(1, std::memory_order_relaxed);
                                     }];
    } else if (sMotion != nil && sMotion.deviceMotionActive) {
        [sMotion stopDeviceMotionUpdates];
        Ship::TouchControllerState::Instance().gyroX.store(0.0f);
        Ship::TouchControllerState::Instance().gyroY.store(0.0f);
    }
}
#endif
