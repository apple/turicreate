//
//  mps_device_manager.m
//  tcmps
//
//  Copyright © 2018 Turi. All rights reserved.
//

#import "mps_device_manager.h"

NS_ASSUME_NONNULL_BEGIN

@interface TCMPSDeviceManager ()
{
  // Token received from Metal to control notifications
  id <NSObject> _deviceObserver;
}

// Internally re-declare this property as writable
@property(readwrite, nullable, atomic) id <MTLDevice> preferredDevice;

@end

@implementation TCMPSDeviceManager

+ (instancetype)sharedInstance {
  static TCMPSDeviceManager *singleton = nil;
  static dispatch_once_t onceToken;
  dispatch_once(&onceToken, ^{
    singleton = [[TCMPSDeviceManager alloc] init];
  });
  return singleton;
}

- (instancetype)init {
  self = [super init];
  if (self) {
    // Define a notification handler that simply recomputes the preferred device
    // from scratch on each change to the device list.
    __weak TCMPSDeviceManager *weakSelf = self;
    MTLDeviceNotificationHandler handler = ^(id <MTLDevice> device, MTLDeviceNotificationName name) {
      [weakSelf setPreferredDeviceFromDevices:MTLCopyAllDevices()];
      // (In theory we should only set self.preferredDevice on our own serial
      // queue to guard against race conditions, but here it doesn't seem worth
      // the cost in complexity.)
    };

    // Retrieve the initial device list and register for change notifications.
    id <NSObject> deviceObserver = nil;
    NSArray<id <MTLDevice>> *devices = MTLCopyAllDevicesWithObserver(&deviceObserver, handler);
    _deviceObserver = deviceObserver;

    // Set the initial preferred device.
    [self setPreferredDeviceFromDevices:devices];
  }
  return self;
}

- (void)dealloc {
  MTLRemoveDeviceObserver(_deviceObserver);
}

- (BOOL)shouldPreferDevice:(id <MTLDevice>)candidate overDevice:(nullable id <MTLDevice>)current {
  // Something is better than nothing.
  if (!current) return YES;

  // Prefer high-power devices over low-power devices.
  if (!candidate.isLowPower && current.isLowPower) return YES;
  if (candidate.isLowPower && !current.isLowPower) return NO;

  // Otherwise, prefer external GPUs.
  if (candidate.isRemovable && !current.isRemovable) return YES;
  if (!candidate.isRemovable && current.isRemovable) return NO;

  // Otherwise, prefer GPUs not driving a monitor.
  if (candidate.isHeadless && !current.isHeadless) return YES;
  if (!candidate.isHeadless && current.isHeadless) return NO;

  // Otherwise, arbitrarily prefer earlier devices in the list.
  return NO;
}

- (void)setPreferredDeviceFromDevices:(NSArray<id <MTLDevice>> *)devices {
  // Find the most preferred device.
  id <MTLDevice> preferred = nil;
  for (id <MTLDevice> device in devices) {
    if ([self shouldPreferDevice:device overDevice:preferred]) {
      preferred = device;
    }
  }

  self.preferredDevice = preferred;  // atomic
}

@end

NS_ASSUME_NONNULL_END
