#include <unity/lib/visualization/dark_mode.hpp>

#ifdef __APPLE__
#import <CoreFoundation/CoreFoundation.h>
#endif

namespace turi {
namespace visualization {

bool is_system_dark_mode() {
#ifdef __APPLE__
    CFStringRef key = CFSTR("AppleInterfaceStyle");
    CFStringRef expectedValue = CFSTR("Dark");
    CFPropertyListRef propertyList = CFPreferencesCopyValue(
        key,
        kCFPreferencesAnyApplication,
        kCFPreferencesCurrentUser,
        kCFPreferencesAnyHost);
    if (propertyList == nullptr) {
        return false;
    }
    bool ret = CFEqual(propertyList, expectedValue);
    CFRelease(propertyList);
    return ret;
#else
    // TODO - what should this do on other platforms?
    return false;
#endif
}

}
}