/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef UNITY_TOOLKITS_NEURAL_NET_MPS_COMMAND_QUEUE_HPP_
#define UNITY_TOOLKITS_NEURAL_NET_MPS_COMMAND_QUEUE_HPP_

// C++ headers and implementations can include this file for the forward
// declaration of mps_command_queue, allowing C++ code to pass mps_command_queue
// pointers and references. Objective C++ implementations can include this file
// to obtain the MTLCommandQueue that the mps_command_queue wraps.

namespace turi {
namespace neural_net {

// C++-only files just get this forward declaration.
struct mps_command_queue;

}  // namespace neural_net
}  // namespace turi

// Define the mps_command_queue struct for Objective C++ files.
#ifdef __OBJC__

#import <Metal/Metal.h>

NS_ASSUME_NONNULL_BEGIN

namespace turi {
namespace neural_net {

/**
 * Simple wrapper around an MTLCommandQueue-conforming Objective C object, so
 * that pure C++ code can pass these objects around.
 */
struct mps_command_queue {
  id <MTLCommandQueue> impl = nil;
};

}  // namespace neural_net
}  // namespace turi

NS_ASSUME_NONNULL_END

#endif  // __OBJC__

#endif  // UNITY_TOOLKITS_NEURAL_NET_MPS_COMMAND_QUEUE_HPP_
