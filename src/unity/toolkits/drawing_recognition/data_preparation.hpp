/* Copyright © 2019 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_DRAWING_DATA_PREPARATION_H_
#define TURI_DRAWING_DATA_PREPARATION_H_

#include <export.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <unity/lib/variant.hpp>
#include <unity/lib/image_util.hpp>

namespace turi {
namespace drawing_recognition {

/**
 *  This function is responsible for converting stroke-based drawing data
 *  to 28x28 bitmaps to make it ready for training with the Neural Network.
 *  
 * 
 * \param[in] data 					SFrame from the user, which contains 
 *									stroke-based drawings
 * \param[in] feature				Name of the feature column
 * \param[in] target				Name of the target (label) column
 * 
 * 
 *
 * \return                          SFrame with the stroke-based drawings 
 *									converted to bitmaps.
 */
EXPORT gl_sframe _drawing_recognition_prepare_data(const gl_sframe &data,
                                                   const std::string &feature,
                                                   const std::string &target);


}
}

#endif //TURI_DRAWING_DATA_PREPARATION_H_
