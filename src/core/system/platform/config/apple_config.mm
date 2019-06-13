#ifdef __APPLE__

#import <Foundation/Foundation.h>
#include <string>
#include <core/system/platform/config/apple_config.hpp>

@interface DummyThread : NSObject {}
@end

@implementation DummyThread

- (void)dummy {
  // Just return
}

- (instancetype)init {
    NSThread* _thread = [[NSThread alloc] initWithTarget:self selector:@selector(dummy) object:nil];
    [_thread start];
    return self;
}

@end

namespace turi { namespace config {

static bool _cocoa_initialized = false;

void init_cocoa_multithreaded_runtime() {

  if(_cocoa_initialized) {
    return;
  }

  // Start an NSThread and let it exit.  This turns the CoreFoundation / Cocoa
  // runtime to multithreaded mode.  Otherwise, shit is bad in weird ways.
  @autoreleasepool {
     [[DummyThread alloc] init];
  };

 _cocoa_initialized = true;
}

std::string get_apple_system_temporary_directory() {

  @autoreleasepool {
    return [NSTemporaryDirectory() UTF8String];
  }

}

}}

#endif
