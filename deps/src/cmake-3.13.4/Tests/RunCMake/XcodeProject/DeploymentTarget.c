#include <Availability.h>
#include <TargetConditionals.h>

#if TARGET_OS_OSX
#  if __MAC_OS_X_VERSION_MIN_REQUIRED != __MAC_10_11
#    error macOS deployment version mismatch
#  endif
#elif TARGET_OS_IOS
#  if __IPHONE_OS_VERSION_MIN_REQUIRED != __IPHONE_9_1
#    error iOS deployment version mismatch
#  endif
#elif TARGET_OS_WATCH
#  if __WATCH_OS_VERSION_MIN_REQUIRED != __WATCHOS_2_0
#    error watchOS deployment version mismatch
#  endif
#elif TARGET_OS_TV
#  if __TV_OS_VERSION_MIN_REQUIRED != __TVOS_9_0
#    error tvOS deployment version mismatch
#  endif
#else
#  error unknown OS
#endif

void foo()
{
}
