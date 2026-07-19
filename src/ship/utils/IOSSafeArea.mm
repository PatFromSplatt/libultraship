#ifdef __IOS__
#import <UIKit/UIKit.h>

// Safe-area insets in DRAWABLE PIXELS (UIKit points premultiplied by the screen's native
// scale), matching the coordinate space of ImGui's DisplaySize on the Metal backend.
extern "C" void GetIOSSafeAreaInsets(float* top, float* left, float* bottom, float* right) {
    UIWindow* window = UIApplication.sharedApplication.windows.firstObject;
    const UIEdgeInsets insets = window != nil ? window.safeAreaInsets : UIEdgeInsetsZero;
    const CGFloat scale = UIScreen.mainScreen.nativeScale;
    *top = (float)(insets.top * scale);
    *left = (float)(insets.left * scale);
    *bottom = (float)(insets.bottom * scale);
    *right = (float)(insets.right * scale);
}
#endif
