//
//  mps_dev.mm
//  tcmps
//
//  Copyright © 2018 Turi. All rights reserved.
//

#import "mps_dev.h"

int devicePriority(id<MTLDevice> dev) {
  int prio = 0;
  if (!dev.isLowPower) {
    prio += 100;
  }

  if (dev.isRemovable) {
    prio += 10;
  }

  if (dev.isHeadless) {
    prio += 1;
  }
  return prio;
}
