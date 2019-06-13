/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef FEATURE_ENGINEERING_REGISTRATIONS_H_
#define FEATURE_ENGINEERING_REGISTRATIONS_H_


#include <unity/lib/toolkit_class_macros.hpp>
#include <unity/lib/toolkit_function_macros.hpp>

#include <unity/toolkits/feature_engineering/sample_transformer.hpp>
#include <unity/toolkits/feature_engineering/dimension_reduction.hpp>
#include <unity/toolkits/feature_engineering/quadratic_features.hpp>
#include <unity/toolkits/feature_engineering/one_hot_encoder.hpp>
#include <unity/toolkits/feature_engineering/count_thresholder.hpp>
#include <unity/toolkits/feature_engineering/feature_binner.hpp>
#include <unity/toolkits/feature_engineering/mean_imputer.hpp>
#include <unity/toolkits/feature_engineering/tfidf.hpp>
#include <unity/toolkits/feature_engineering/bm25.hpp>
#include <unity/toolkits/feature_engineering/tokenizer.hpp>
#include <unity/toolkits/feature_engineering/word_counter.hpp>
#include <unity/toolkits/feature_engineering/ngram_counter.hpp>
#include <unity/toolkits/feature_engineering/word_trimmer.hpp>
#include <unity/toolkits/feature_engineering/categorical_imputer.hpp>
#include <unity/toolkits/feature_engineering/count_featurizer.hpp>
#include <unity/toolkits/feature_engineering/transform_to_flat_dictionary.hpp>

namespace turi {
namespace sdk_model {
namespace feature_engineering {

BEGIN_CLASS_REGISTRATION
REGISTER_CLASS(sample_transformer)
REGISTER_CLASS(random_projection)
REGISTER_CLASS(quadratic_features)
REGISTER_CLASS(one_hot_encoder)
REGISTER_CLASS(count_thresholder)
REGISTER_CLASS(feature_binner)
REGISTER_CLASS(mean_imputer)
REGISTER_CLASS(tfidf)
REGISTER_CLASS(bm25)
REGISTER_CLASS(tokenizer)
REGISTER_CLASS(word_counter)
REGISTER_CLASS(ngram_counter)
REGISTER_CLASS(categorical_imputer)
REGISTER_CLASS(count_featurizer)
REGISTER_CLASS(word_trimmer)
REGISTER_CLASS(transform_to_flat_dictionary)
END_CLASS_REGISTRATION


}// feature_engineering
}// sdk_model
}// turicreate
#endif
