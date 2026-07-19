#ifdef __IOS__
#import <CoreMotion/CoreMotion.h>
#include "ship/touch/TouchControllerState.h"

static CMMotionManager* sMotion = nil;

// Feeds device rotation-rate into the pad's native gyro fields (the game reads
// OSContPad.gyro_x/gyro_y for first-person aiming, see z_player.c). Values are small
// per-frame rates; the game applies its own large scale factor.
extern "C" void IOSGyroSetEnabled(bool enabled, float sensitivity) {
    if (enabled) {
        if (sMotion == nil) {
            sMotion = [[CMMotionManager alloc] init];
        }
        if (!sMotion.gyroAvailable || sMotion.gyroActive) {
            return;
        }
        sMotion.gyroUpdateInterval = 1.0 / 60.0;
        [sMotion startGyroUpdatesToQueue:[NSOperationQueue mainQueue]
                             withHandler:^(CMGyroData* data, NSError* error) {
                                 if (data == nil) {
                                     return;
                                 }
                                 auto& state = Ship::TouchControllerState::Instance();
                                 // landscape mapping: rotation about the device's long axis
                                 // pans (yaw), about the short axis tilts (pitch)
                                 state.gyroX.store((float)(-data.rotationRate.x) * sensitivity * 0.02f);
                                 state.gyroY.store((float)(-data.rotationRate.z) * sensitivity * 0.02f);
                             }];
    } else if (sMotion != nil && sMotion.gyroActive) {
        [sMotion stopGyroUpdates];
        Ship::TouchControllerState::Instance().gyroX.store(0.0f);
        Ship::TouchControllerState::Instance().gyroY.store(0.0f);
    }
}
#endif
