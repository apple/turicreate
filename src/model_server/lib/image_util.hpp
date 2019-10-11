/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef IMAGE_UTIL_HPP
#define IMAGE_UTIL_HPP

#include <core/data/flexible_type/flexible_type.hpp>
#include <core/storage/sframe_interface/unity_sframe.hpp>

namespace turi{

namespace image_util {

void copy_image_to_memory(const image_type& input, unsigned char* outptr,
                          const std::vector<size_t>& outstrides,
                          const std::vector<size_t>& outshape,
                          bool channel_last);

void copy_image_to_memory(const image_type& input, float* outptr,
                          const std::vector<size_t>& outstrides,
                          const std::vector<size_t>& outshape,
                          bool channel_last);

/**
* Return the head of passed sarray, but cast to string. Used for printing on python side.
*/
std::shared_ptr<unity_sarray> _head_str(std::shared_ptr<unity_sarray> image_sarray, size_t num_rows);

/**
* Return flex_vec flexible type that is sum of all images with data in vector form.
*/

flexible_type sum(std::shared_ptr<unity_sarray> unity_data);

/**
 * Generate image of mean pixel values of images in a unity sarray
 */
flexible_type generate_mean(std::shared_ptr<unity_sarray> unity_data);


/**************************************************************************/
/*                                                                        */
/*                              Image Loader                              */
/*                                                                        */
/**************************************************************************/

/**
 * Construct an sframe of flex_images, with url pointing to directory where images reside.
 */
std::shared_ptr<unity_sframe> load_images(std::string url, std::string format,
    bool with_path, bool recursive, bool ignore_failure, bool random_order);

/**
 * Construct a single image from url, and format hint.
 */
flexible_type load_image(const std::string& url, const std::string format);


/**************************************************************************/
/*                                                                        */
/*                           Encode and Decode                            */
/*                                                                        */
/**************************************************************************/


/**
 * Decode the image into raw pixels
 */
flexible_type decode_image(const flexible_type& data);

/**
 * Decode an sarray of flex_images into raw pixels
 */
std::shared_ptr<unity_sarray> decode_image_sarray(
    std::shared_ptr<unity_sarray> image_sarray);

/**
 * Encode the image into compressed format (losslessly)
 * No effect on already encoded images, even if the format is different.
 */
flexible_type encode_image(const flexible_type& data);


/**************************************************************************/
/*                                                                        */
/*                                 Resize                                 */
/*                                                                        */
/**************************************************************************/

/** Resize an sarray of flex_images with the new size. The sampling method
 * is specified as the polynomial order of the resampling kernel, with 0
 * (nearest neighbor) and 1 (bilinear) supported.
 */
flexible_type resize_image(const flexible_type& image, size_t resized_width,
    size_t resized_height, size_t resized_channel, bool decode = false,
    int resample_method = 0);

/** Resize an sarray of flex_image with the new size.
 */
std::shared_ptr<unity_sarray> resize_image_sarray(
    std::shared_ptr<unity_sarray> image_sarray, size_t resized_width,
    size_t resized_height, size_t resized_channels, bool decode = false,
    int resample_method = 0);



/**************************************************************************/
/*                                                                        */
/*                      Vector <-> Image Conversion                       */
/*                                                                        */
/**************************************************************************/

/** Convert sarray of image data to sarray of vector
 */
std::shared_ptr<unity_sarray>
  image_sarray_to_vector_sarray(std::shared_ptr<unity_sarray> image_sarray,
      bool undefined_on_failure);

/** Convert sarray of vector to sarray of image
 */
std::shared_ptr<unity_sarray>
  vector_sarray_to_image_sarray(std::shared_ptr<unity_sarray> image_sarray,
      size_t width, size_t height, size_t channels, bool undefined_on_failure);

}  // namespace image_util

} // end of turicreate

#endif /* IMAGE_UTIL_HPP*/
