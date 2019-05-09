/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <boost/gil/gil_all.hpp>

#include <image/image_type.hpp>

#include <unity/lib/gl_sframe.hpp>
#include <unity/toolkits/object_detection/one_shot_object_detection/util/parameter_sampler.hpp>

namespace turi {
namespace one_shot_object_detection {

namespace data_augmentation {

namespace superposition {

flex_image create_synthetic_rgb_image(const boost::gil::rgb8_image_t::view_t &background_view,
                                      ParameterSampler &parameter_sampler,
                                      const flex_image &object);

flex_image create_synthetic_rgba_image(const boost::gil::rgb8_image_t::view_t &background_view, 
                                       ParameterSampler &parameter_sampler,
                                       const flex_image &object);

} // superposition
} // data_augmentation
} // one_shot_object_detection
} // turi