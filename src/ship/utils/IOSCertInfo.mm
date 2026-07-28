#ifdef __IOS__
#import <Foundation/Foundation.h>
#include <cstdint>
#include <string>

// Free-Apple-ID sideloads carry an embedded.mobileprovision whose ExpirationDate is the day
// the app stops launching (7 days for a free account). The file is a CMS/DER envelope, but the
// plist inside is plain text — a string scan is enough, no CMS parsing. If the file is absent
// (simulator, TrollStore, enterprise) return 0 and the UI hides the row entirely.
extern "C" int64_t IOSGetSignatureExpiryUnix(void) {
    static int64_t sCached = -2; // -2 = not computed yet; 0 = unknown/absent
    if (sCached != -2) {
        return sCached;
    }
    sCached = 0;
    @autoreleasepool {
        NSString* path = [[NSBundle mainBundle] pathForResource:@"embedded" ofType:@"mobileprovision"];
        if (path == nil) {
            return sCached;
        }
        NSData* data = [NSData dataWithContentsOfFile:path];
        if (data == nil || data.length == 0) {
            return sCached;
        }
        const std::string haystack(static_cast<const char*>(data.bytes), data.length);
        const std::string key = "<key>ExpirationDate</key>";
        size_t at = haystack.find(key);
        if (at == std::string::npos) {
            return sCached;
        }
        const std::string open = "<date>";
        size_t dateStart = haystack.find(open, at);
        size_t dateEnd = haystack.find("</date>", dateStart);
        if (dateStart == std::string::npos || dateEnd == std::string::npos) {
            return sCached;
        }
        dateStart += open.size();
        NSString* iso = [[NSString alloc] initWithBytes:haystack.data() + dateStart
                                                 length:dateEnd - dateStart
                                               encoding:NSUTF8StringEncoding];
        if (iso == nil) {
            return sCached;
        }
        NSISO8601DateFormatter* fmt = [[NSISO8601DateFormatter alloc] init];
        NSDate* date = [fmt dateFromString:iso];
        if (date != nil) {
            sCached = (int64_t)[date timeIntervalSince1970];
        }
    }
    return sCached;
}
#endif
