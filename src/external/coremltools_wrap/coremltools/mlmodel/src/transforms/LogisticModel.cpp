/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include "LogisticModel.hpp"
#include "../Format.hpp"

namespace CoreML {

  LogisticModel::LogisticModel(const std::string& predictedClassOutputName,
                               const std::string& classProbabilityOutputName,
                               const std::string& description)
    : Model(description) {
      Specification::GLMClassifier* lr = m_spec->mutable_glmclassifier();
        lr->set_postevaluationtransform(Specification::GLMClassifier::Logit);
        m_spec->mutable_description()->set_predictedfeaturename(predictedClassOutputName);
        m_spec->mutable_description()->set_predictedprobabilitiesname(classProbabilityOutputName);
    }

  Result LogisticModel::setWeights(const std::vector< std::vector<double> >& weights) {
    Specification::GLMClassifier* lr = m_spec->mutable_glmclassifier();
    for(auto w : weights) {
      CoreML::Specification::GLMClassifier::DoubleArray* cur_arr = lr->add_weights();
      for(double n : w) {
        cur_arr->add_value(n);
      }
    }
    return Result();
  }

  Result LogisticModel::setOffsets(const std::vector<double>& offsets) {
    Specification::GLMClassifier* lr = m_spec->mutable_glmclassifier();
    for(double n : offsets) {
      lr->add_offset(n);
    }
    return Result();
  }

    /**  Set up the class list -- two versions for string and integer values.
     */
    Result LogisticModel::setClassNames(const std::vector<std::string>& classes) {
        m_spec->mutable_glmclassifier()->mutable_stringclasslabels()->clear_vector();
        for(size_t i = 0; i < classes.size(); ++i) {
            std::string *category = m_spec->mutable_glmclassifier()->mutable_stringclasslabels()->add_vector();
            *category = classes[i];
        }
        return Result();
    }

    Result LogisticModel::setClassNames(const std::vector<int64_t>& classes) {
        m_spec->mutable_glmclassifier()->mutable_int64classlabels()->clear_vector();
        for(size_t i = 0; i < classes.size(); ++i) {
            m_spec->mutable_glmclassifier()->mutable_int64classlabels()->add_vector(classes[i]);
        }
        return Result();
    }
}
