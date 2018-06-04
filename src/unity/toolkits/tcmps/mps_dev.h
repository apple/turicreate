//
//  mps_dev.h
//  tcmps
//
//  Copyright © 2018 Turi. All rights reserved.
//

#ifndef mps_dev_h
#define mps_dev_h

#import <Metal/Metal.h>
#import <MetalPerformanceShaders/MetalPerformanceShaders.h>
#import <Foundation/Foundation.h>
#import <memory>
#import <mutex>

template<typename T>
struct ThreadLocal {
public:
  static T* Get() {
    static thread_local T inst;
    return &inst;
  }
private:
  ThreadLocal() {};
};

// Determine priority of a device, used if more than one is available
int devicePriority(id<MTLDevice> dev);

struct MetalDefaultDevice {
  id<MTLDevice> dev {nil};
  MetalDefaultDevice() {
    NSArray<id<MTLDevice>> *unorderedDevices = MTLCopyAllDevices();
    NSArray<id<MTLDevice>> *devices = [unorderedDevices
      sortedArrayUsingComparator:^(id<MTLDevice> a1, id<MTLDevice> a2) {
        int a1_prio = devicePriority(a1);
        int a2_prio = devicePriority(a2);
        // Negative for descending priority
        return [@(-a1_prio) compare:@(-a2_prio)];
      }];
    assert([devices count] > 0 && "Should make sure device is available first before creating context");
    // Device index 0 is now the top priority device
    dev = [devices objectAtIndex:0];
  }
};

typedef ThreadLocal<MetalDefaultDevice> MetalDevice;
#endif /* mps_dev_h */
