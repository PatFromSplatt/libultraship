#ifdef __IOS__
#import <UIKit/UIKit.h>

// Safe-area insets in UIKit POINTS — the same coordinate space as ImGui's DisplaySize
// under the SDL2 backend (which reports window points and carries the retina factor in
// DisplayFramebufferScale instead).
extern "C" void GetIOSSafeAreaInsets(float* top, float* left, float* bottom, float* right) {
    UIWindow* window = UIApplication.sharedApplication.windows.firstObject;
    const UIEdgeInsets insets = window != nil ? window.safeAreaInsets : UIEdgeInsetsZero;
    *top = (float)insets.top;
    *left = (float)insets.left;
    *bottom = (float)insets.bottom;
    *right = (float)insets.right;
}
#endif
