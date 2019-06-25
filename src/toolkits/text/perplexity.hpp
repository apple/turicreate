/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_TEXT_PERPLEXITY_H_
#define TURI_TEXT_PERPLEXITY_H_

#include <core/storage/sframe_interface/unity_sarray.hpp>

namespace turi {
namespace text {

/**
 * Compute perplexity, a measure of the likelihood of data given
 * the current parameters of the model.

 *  Then for each word in each document, we compute
 *  \f[ \Pr(word | theta[doc_id,:], phi[word,:]) =
 *     \sum_k theta[doc_id, k] * phi[word_id, k] \f]
 *
 *  We compute loglikelihood to be:
 *  \f[l(D) = \sum_{i \in D} \sum_{j in D_i} count_{i,j} *
 *   log Pr(word_{i,j} | \theta, \phi)\f]
 *
 *  and perplexity to be
 *  \f[\exp \{ - l(D) / \sum_i \sum_j count_{i,j} \}\f]
 *
 *  For more information, see http://en.wikipedia.org/wiki/Perplexity.
 */
double perplexity(std::shared_ptr<sarray<flexible_type>> documents,
    const std::shared_ptr<sarray<flexible_type>> doc_topic_prob,
    const std::shared_ptr<sarray<flexible_type>> word_topic_prob,
    const std::shared_ptr<sarray<flexible_type>> vocabulary);

} // text
} // turicreate

#endif
