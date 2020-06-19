/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#define BOOST_TEST_MODULE test_image_augmentation

#include <ml/neural_net/image_augmentation.hpp>

#include <algorithm>
#include <vector>

#include <boost/test/unit_test.hpp>
#include <core/util/test_macros.hpp>

namespace turi {
namespace neural_net {
namespace {

BOOST_AUTO_TEST_CASE(test_image_box_constructor) {
  // Default boxes are zero-initialized.
  image_box box;
  TS_ASSERT_EQUALS(box.x, 0.f);
  TS_ASSERT_EQUALS(box.y, 0.f);
  TS_ASSERT_EQUALS(box.width, 0.f);
  TS_ASSERT_EQUALS(box.height, 0.f);

  // The constructor takes arguments in x,y,width,height order.
  box = image_box(1.f, 2.f, 3.f, 4.f);  
  TS_ASSERT_EQUALS(box.x, 1.f);
  TS_ASSERT_EQUALS(box.y, 2.f);
  TS_ASSERT_EQUALS(box.width, 3.f);
  TS_ASSERT_EQUALS(box.height, 4.f);
}

BOOST_AUTO_TEST_CASE(test_image_box_area) {
  // Typical case
  image_box box(0.f, 0.f, 0.4f, 0.5f);
  TS_ASSERT_EQUALS(box.area(), 0.2f);

  // Any negative width or height yields zero area.
  box = image_box(1.f, 1.f, -0.5f, 0.5f);
  TS_ASSERT_EQUALS(box.area(), 0.f);
  box = image_box(1.f, 1.f, 0.5f, -0.5f);
  TS_ASSERT_EQUALS(box.area(), 0.f);
}

BOOST_AUTO_TEST_CASE(test_image_box_normalize) {
  // Typical case
  image_box box(10.f, 20.f, 30.f, 40.f);
  box.normalize(100.f, 50.f);
  TS_ASSERT_EQUALS(box, image_box(0.1f, 0.4f, 0.3f, 0.8f));
}

BOOST_AUTO_TEST_CASE(test_image_box_clip) {
  image_box box;

  // Clipping to a larger box is a no-op.
  box = image_box(10.f, 20.f, 30.f, 40.f);
  box.clip(image_box(0.f, 0.f, 100.f, 100.f));
  TS_ASSERT_EQUALS(box, image_box(10.f, 20.f, 30.f, 40.f));

  // Clipping to a strictly contained box results in the contained box.
  box = image_box(10.f, 20.f, 30.f, 40.f);
  box.clip(image_box(15.f, 25.f, 10.f, 10.f));
  TS_ASSERT_EQUALS(box, image_box(15.f, 25.f, 10.f, 10.f));

  // Clipping to an overlapping box returns the intersection.
  box = image_box(10.f, 20.f, 30.f, 40.f);
  box.clip(image_box(20.f, 0.f, 10.f, 80.f));
  TS_ASSERT_EQUALS(box, image_box(20.f, 20.f, 10.f, 40.f));

  // Clipping to a non-overlapping box return an empty box.
  // Clipping to a strictly contained box results in the contained box.
  box = image_box(10.f, 20.f, 30.f, 40.f);
  box.clip(image_box(70.f, 70.f, 100.f, 100.f));
  TS_ASSERT_EQUALS(box.area(), 0.f);
}

}  // namespace
}  // neural_net
}  // turi
