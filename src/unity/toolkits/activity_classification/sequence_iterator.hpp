/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SEQUENCE_ITERATOR_H_
#define TURI_SEQUENCE_ITERATOR_H_

#include <export.hpp>
//#include <sframe/sframe.hpp>
#include <unity/lib/gl_sframe.hpp>
#include <unity/lib/variant.hpp>


namespace turi {
namespace sdk_model {
namespace activity_classification {

class sequence_iterator {

};


/**
 *  Convert SFrame to batch form, where each row contains a sequence of length
 *  predictions_in_chunk * prediction_window, and there is a single label per
 *  prediction window.
 *
 * \param[in] data                  Original data. Sframe containing one line per time sample.
 * \param[in] features              List of names of the columns containing the input features.
 * \param[in] session_id            Name of the column containing ids for each session in the dataset.
 *                                  A session is a single user time-series sequence.
 * \param[in] prediction_window     Number of time samples in every prediction window. A label is expected
 *                                  (for training), or predicted (in inference) every time a sequence of
 *                                  prediction_window samples have been collected.
 * \param[in] predictions_in_chunk  Each session is chunked into shorter sequences. This is the number of
 *                                  prediction windows desired in each chunk.
 * \param[in] target                Name of the coloumn containing the output labels. Empty string if None.
 *
 * \return                          SFrame with the data converted to batch form.
 */
EXPORT variant_map_type _activity_classifier_prepare_data(const gl_sframe &data,
                                                   const std::vector<std::string> &features,
                                                   const std::string &session_id,
                                                   const int &prediction_window,
                                                   const int &predictions_in_chunk,
                                                   const std::string &target);


// Same as above, with verbose=True
EXPORT variant_map_type _activity_classifier_prepare_data_verbose(const gl_sframe &data,
                                                   const std::vector<std::string> &features,
                                                   const std::string &session_id,
                                                   const int &prediction_window,
                                                   const int &predictions_in_chunk,
                                                   const std::string &target);
}
}
}

#endif //TURI_SEQUENCE_ITERATOR_H_
