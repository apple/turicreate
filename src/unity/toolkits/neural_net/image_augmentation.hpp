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

  bool empty() const { return width <= 0.f || height <= 0.f; }

  // Computes the area if the width and height are positive, otherwise returns 0
  float area() const {
    return empty() ? 0.f : (width * height);
  }

  // Divides each coordinate and length by the appropriate normalizer.
  void normalize(float image_width, float image_height);

  // Sets this instance to the intersection with the given image_box. If no
  // intersection exists, then the result will have area() of 0.f (and may have
  // negative width or height).
  void clip(image_box clip_box = image_box(0.f, 0.f, 1.f, 1.f));

  // Grows this instance (minimally) so that its area contains the (non-empty)
  // area of the other image_box.
  void extend(const image_box& other);

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

  // Used when parsing saved predictions for evaluation.
  std::vector<image_annotation> predictions;
};

/**
 * Pure virtual interface for objects that process/augment/mutate images and
 * their associated annotations.
 */
class image_augmenter {
public:

  /** Parameters governing random crops. */
  struct crop_options {

    /** Lower bound for the uniformly sampled aspect ratio (width/height) */
    float min_aspect_ratio = 0.8f;

    /** Upper bound for the uniformly sampled aspect ratio (width/height) */
    float max_aspect_ratio = 1.25f;

    /**
     * Given a sampled aspect ratio, determines the lower bound of the uniformly
     * sampled height.
     */
    float min_area_fraction = 0.15f;

    /**
     * Given a sampled aspect ratio, determines the upper bound of the uniformly
     * sampled height.
     */
    float max_area_fraction = 1.f;

    /**
     * Given a sampled crop (aspect ratio, height, and location), specifies the
     * minimum fraction of each bounding box's area that must be included to
     * accept the crop. If 0.f, then the crop need not touch any object.
     */
    float min_object_covered = 0.f;

    /**
     * The maximum number of random crops to sample in an attempt to generate
     * one that satisfies the min_object_covered constraint.
     */
    size_t max_attempts = 50;

    /**
     * Given an accepted crop, the minimum fraction of each bounding box's area
     * that must be included to keep the (potentially cropped) bounding box in
     * the annotations (instead of discarding it).
     */
    float min_eject_coverage = 0.5f;
  };

  /** Parameters governing random padding. */
  struct pad_options {

    /** Lower bound for the uniformly sampled aspect ratio (width/height) */
    float min_aspect_ratio = 0.8f;

    /** Upper bound for the uniformly sampled aspect ratio (width/height) */
    float max_aspect_ratio = 1.25f;

    /**
     * Given a sampled aspect ratio, determines the lower bound of the uniformly
     * sampled height.
     */
    float min_area_fraction = 1.f;

    /**
     * Given a sampled aspect ratio, determines the upper bound of the uniformly
     * sampled height.
     */
    float max_area_fraction = 2.f;

    /**
     * The maximum number of random aspect ratios to sample, looking for one
     * that satisfies the constraints on area.
     */
    size_t max_attempts = 50;
  };

  /**
   * Parameters for constructing new image_augmenter instances.
   *
   * Default constructed values perform no augmentation, outside of resizing to
   * the output width and height (which must be specified).
   */
  struct options {

    /** The N dimension of the resulting float array. */
    size_t batch_size = 0;

    /** The W dimension of the resulting float array. */
    size_t output_width = 0;

    /** The H dimension of the resulting float array. */
    size_t output_height = 0;

    /** The probability of applying (attempting) a random crop. */
    float crop_prob = 0.f;
    crop_options crop_opts;

    /** The probability of applying (attempting) a random pad. */
    float pad_prob = 0.f;
    pad_options pad_opts;

    /** The probability of flipping the image horizontally. */
    float horizontal_flip_prob = 0.f;

    // TODO: The semantics below are adopted from Core Image but don't match the
    // behavior of MXNet augmentations.... What should a shared interface
    // specify?
    // See also https://developer.apple.com/library/archive/documentation/GraphicsImaging/Reference/CoreImageFilterReference/index.html#//apple_ref/doc/filter/ci/CIColorControls

    /**
     * Maximum pixel value to add or subtract to each channel.
     *
     * For example, a value of 0.05 adds a random value between -0.05 and 0.05
     * to each channel of each pixel (represented as a value from 0 to 1).
     */
    float brightness_max_jitter = 0.f;

    /**
     * Maximum proportion to increase or decrease contrast.
     *
     * For example, a value of 0.05 multiplies the contrast by a random value
     * between 0.95 and 1.05.
     */
    float contrast_max_jitter = 0.f;

    /**
     * Maximum proportion to increase or decrease saturation.
     *
     * For example, a value of 0.05 multiplies the saturation by a random value
     * between 0.95 and 1.05.
     */
    float saturation_max_jitter = 0.f;

    /**
     * Maximum proportion to rotate the hues.
     *
     * For example, a value of 0.05 applies a random rotation between
     * -0.05 * pi and 0.05 * pi.
     */
    float hue_max_jitter = 0.f;
  };

  /** The output of an image_augmenter. */
  struct result {

    /** The augmented images, represented as a single NHWC array (RGB). */
    shared_float_array image_batch;

    /**
     * The transformed annotations for each augmented image. This vector's size
     * should equal the size of the source batch that generated the result, and
     * each inner vector should have the same length as the corresponding input
     * image's annotations vector. */
    std::vector<std::vector<image_annotation>> annotations_batch;
  };

  virtual ~image_augmenter() = default;

  /** Returns the options parameterizing this instance. */
  virtual const options& get_options() const = 0;

  /**
   * Performs augmentation on a batch of images (and their annotations).
   *
   * If the source batch is smaller than the batch size specified in the
   * options, then the result is padded with zeroes as needed.
   */
  virtual result prepare_images(std::vector<labeled_image> source_batch) = 0;
};

/**
 * An image_augmenter implementation that only resizes the input images to the
 * desired output shape.
 */
class resize_only_image_augmenter final: public image_augmenter {
public:
  explicit resize_only_image_augmenter(const options& opts): opts_(opts) {}

  const options& get_options() const override { return opts_; }

  result prepare_images(std::vector<labeled_image> source_batch) override;

private:
  options opts_;
};

}  // neural_net
}  // turi

#endif  // TURI_NEURAL_NET_IMAGE_AUGMENTATION_HPP_
