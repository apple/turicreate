/* Copyright © 2020 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at
 * https://opensource.org/licenses/BSD-3-Clause
 */

#include <toolkits/util/image_utils.hpp>

#include <ml/neural_net/CoreImageImage.hpp>
#include <model_server/lib/image_util.hpp>

#import <CoreImage/CoreImage.h>

namespace turi {

/// Converts from the Turi image_type to the neural_net Image type.
std::shared_ptr<neural_net::Image> wrap_image(const image_type& image)
{
  @autoreleasepool {
    // Wrap the image with shared_ptr for memory management by NSData below. If the image happens to
    // be unencoded, encode it along the way.
    __block auto shared_image =
        std::make_shared<image_type>(image_util::encode_image(image).to<image_type>());

    // Wrap the (shared) image data with NSData.
    auto deallocator = ^(void* bytes, NSUInteger length) {
      shared_image.reset();
    };
    void* data = const_cast<unsigned char*>(shared_image->get_image_data());
    NSData* imageData = [[NSData alloc] initWithBytesNoCopy:data
                                                     length:shared_image->m_image_data_size
                                                deallocator:deallocator];

    // Let CIImage inspect the file format and decode it.
    CIImage* result = [CIImage imageWithData:imageData];
    if (!result) {
      log_and_throw("Image decoding error");
    }

    return std::make_shared<neural_net::CoreImageImage>(result);
  }  // @autoreleasepool
}

}  // namespace turi
