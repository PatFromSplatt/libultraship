#ifdef __IOS__
#import <UIKit/UIKit.h>

// The app-delegate (non-scene) SDL2 lifecycle still gets one implicit UIWindowScene on iOS 13+.
// Prefer the foreground-active scene, but fall back to any window scene: it may not be active
// yet on the first frames after launch, and the touch overlay caches the insets it sees then.
static UIWindow* SafeAreaWindow(void) {
    UIWindowScene* windowScene = nil;
    for (UIScene* scene in UIApplication.sharedApplication.connectedScenes) {
        if (![scene isKindOfClass:[UIWindowScene class]]) {
            continue;
        }
        if (scene.activationState == UISceneActivationStateForegroundActive) {
            windowScene = (UIWindowScene*)scene;
            break;
        }
        if (windowScene == nil) {
            windowScene = (UIWindowScene*)scene;
        }
    }
    if (windowScene == nil) {
        return nil;
    }
    return windowScene.keyWindow != nil ? windowScene.keyWindow : windowScene.windows.firstObject;
}

// Safe-area insets in UIKit POINTS — the same coordinate space as ImGui's DisplaySize
// under the SDL2 backend (which reports window points and carries the retina factor in
// DisplayFramebufferScale instead).
extern "C" void GetIOSSafeAreaInsets(float* top, float* left, float* bottom, float* right) {
    UIWindow* window = SafeAreaWindow();
    const UIEdgeInsets insets = window != nil ? window.safeAreaInsets : UIEdgeInsetsZero;
    *top = (float)insets.top;
    *left = (float)insets.left;
    *bottom = (float)insets.bottom;
    *right = (float)insets.right;
}
#endif
