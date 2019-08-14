/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_NEURAL_NET_CORE_IMAGE_AUGMENTATION_HPP_
#define TURI_NEURAL_NET_CORE_IMAGE_AUGMENTATION_HPP_

#include <ml/neural_net/image_augmentation.hpp>

#import <CoreImage/CoreImage.h>
#import <ml/neural_net/TCMPSImageAugmenting.h>

namespace turi {
namespace neural_net {

/**
 * Implementation of image_augmentation that uses Core Image, for use with the
 * MPS-based neural-net backend.
 */
class API_AVAILABLE(macos(10.13)) mps_image_augmenter: public image_augmenter {
public:

  explicit mps_image_augmenter(const options& opts);

  // Variant constructor allowing injection of the random number generator,
  // largely for testing.
  mps_image_augmenter(
      const options& opts,
      std::function<float(float lower_bound, float upper_bound)> rng);

  const options& get_options() const override { return opts_; }

  result prepare_images(std::vector<labeled_image> source_batch) override;

private:

  mps_image_augmenter(const options& opts,
                      NSArray<TCMPSUniformRandomNumberGenerator> *rng_batch);

  options opts_;
  CIContext *context_ = nil;
  NSArray<id <TCMPSImageAugmenting>> *augmentations_ = nil;
  NSArray<TCMPSUniformRandomNumberGenerator> *rng_batch_ = nil;
};

}  // neural_net
}  // turi

#endif  // TURI_NEURAL_NET_CORE_IMAGE_AUGMENTATION_HPP_
