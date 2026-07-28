#ifdef __IOS__
#import <AVFoundation/AVFoundation.h>

// A phone call, Siri, or an alarm interrupts the audio session, and iOS does not always hand
// it back — the classic failure is a 10-second call leaving the game permanently silent until
// restart. SDL's CoreAudio backend handles most interruptions itself; this is the belt to its
// braces: an explicit, idempotent re-activation on every return to the foreground. If the
// session is already active this is a no-op; if it was stolen, this takes it back.
extern "C" void IOSAudioSessionReactivate(void) {
    @autoreleasepool {
        NSError* err = nil;
        [[AVAudioSession sharedInstance] setActive:YES error:&err];
        (void)err; // nothing actionable on failure; the next foreground tries again
    }
}
#endif
