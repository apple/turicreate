/* Copyright © 2018 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_NEURAL_NET_IMAGE_AUGMENTATION_HPP_
#define TURI_NEURAL_NET_IMAGE_AUGMENTATION_HPP_

#include <memory>
#include <ostream>
#include <vector>

#include <image/image_type.hpp>
#include <unity/toolkits/neural_net/float_array.hpp>

namespace turi {
namespace neural_net {

/**
 * Represents a rectangular area within an image.
 *
 * The coordinate system is defined by the user. Any rect without a positive
 * width and a positive height is an empty or null rect.
 */
struct image_box {
  image_box() = default;
  image_box(float x, float y, float width, float height)
    : x(x), y(y), width(width), height(height)
  {}

  // Computes the area if the width and height are positive, otherwise returns 0
  float area() const {
    if (width < 0.f || height < 0.f) return 0.f;
    return width * height;
  }

  // Divides each coordinate and length by the appropriate normalizer.
  void normalize(float image_width, float image_height);

  // Sets this instance to the intersection with the given image_box. If no
  // intersection exists, then the result will have area() of 0.f (and may have
  // negative width or height).
  void clip(image_box clip_box = image_box(0.f, 0.f, 1.f, 1.f));

  float x = 0.f;
  float y = 0.f;
  float width = 0.f;
  float height = 0.f;
};

bool operator==(const image_box& a, const image_box& b);
std::ostream& operator<<(std::ostream& out, const image_box& box);

/**
 * Represents a labelled or predicted entity inside an image.
 */
struct image_annotation {
  int identifier = 0;
  image_box bounding_box;
  float confidence = 0.f;  // Typically 1 for training data
};

bool operator==(const image_annotation& a, const image_annotation& b);

/**
 * Contains one image and its associated annotations.
 */
struct labeled_image {
  image_type image;
  std::vector<image_annotation> annotations;
};

/**
 * Pure virtual interface for objects that process/augment/mutate images and
 * their associated annotations.
 */
class image_augmenter {
public:
  /** Parameters for constructing new image_augmenter instances. */
  struct options {
    /** The W dimension of the resulting float array. */
    size_t output_width = 0;

    /** The H dimension of the resulting float array. */
    size_t output_height = 0;
  };

  /** The output of an image_augmenter. */
  struct result {
    /** The augmented images, represented as a single NHWC array (RGB). */
    shared_float_array image_batch;

    /**
     * The transformed annotations for each augmented image. This vector's size
     * should equal the size of the N dimension in the image_batch above, and
     * each inner vector should have the same length as the corresponding input
     * image's annotations vector. */
    std::vector<std::vector<image_annotation>> annotations_batch;
  };

  /**
   * Constructs an image_augmenter. The implementation may depend on platform
   * and hardware resources. */
  static std::unique_ptr<image_augmenter> create(const options& opts);

  virtual ~image_augmenter() = default;

  /** Returns the options parameterizing this instance. */
  virtual const options& get_options() const = 0;

  /**
   * Performs augmentation on a batch of images (and their annotations).
   */
  virtual result prepare_images(std::vector<labeled_image> source_batch) = 0;
};

/**
 * An image_augmenter implementation that only resizes the input images to the
 * desired output shape.
 */
class resize_only_image_augmenter final: public image_augmenter {
public:
  resize_only_image_augmenter(const options& opts): opts_(opts) {}

  const options& get_options() const override { return opts_; }

  result prepare_images(std::vector<labeled_image> source_batch) override;

private:
  options opts_;
};

}  // neural_net
}  // turi

#endif  // TURI_NEURAL_NET_IMAGE_AUGMENTATION_HPP_
