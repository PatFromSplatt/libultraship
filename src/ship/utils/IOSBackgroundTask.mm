#ifdef __IOS__
#import <UIKit/UIKit.h>

// Keeps the process running while the game thread writes its background save, so a suspension
// cannot land mid-write. Main thread only (the game thread is UIKit's main thread under SDL2);
// the expiration handler is delivered there too, so the static needs no synchronisation.
static UIBackgroundTaskIdentifier sBackgroundTask = UIBackgroundTaskInvalid;

extern "C" void IOSEndBackgroundTask(void) {
    if (sBackgroundTask != UIBackgroundTaskInvalid) {
        [[UIApplication sharedApplication] endBackgroundTask:sBackgroundTask];
        sBackgroundTask = UIBackgroundTaskInvalid;
    }
}

extern "C" void IOSBeginBackgroundTask(void) {
    if (sBackgroundTask != UIBackgroundTaskInvalid) {
        return;
    }
    sBackgroundTask = [[UIApplication sharedApplication] beginBackgroundTaskWithExpirationHandler:^{
        IOSEndBackgroundTask();
    }];
}
#endif
