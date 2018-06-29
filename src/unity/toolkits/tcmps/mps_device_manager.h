//
//  mps_device_manager.h
//  tcmps
//
//  Copyright © 2018 Turi. All rights reserved.
//

#import <Foundation/Foundation.h>
#import <Metal/Metal.h>

NS_ASSUME_NONNULL_BEGIN

// Singleton class managing access to Metal devices.
API_AVAILABLE(macos(10.13))
@interface TCMPSDeviceManager : NSObject

// Provides access to the singleton, creating it if necessary.
+ (instancetype)sharedInstance;

// The Metal device to use for MPS computations. Should only be nil if no Metal
// devices are available. This value can change, for example if eGPUs are added
// or removed.
@property(readonly, nullable, atomic) id <MTLDevice> preferredDevice;

@end

NS_ASSUME_NONNULL_END
