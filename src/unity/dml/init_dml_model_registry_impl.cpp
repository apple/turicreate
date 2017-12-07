/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#include <unity/dml/dml_class_registry.hpp>
#include <toolkits/supervised_learning/supervised_learning.hpp>
#include <toolkits/supervised_learning/linear_regression.hpp>
#include <toolkits/supervised_learning/logistic_regression.hpp>
#include <toolkits/supervised_learning/linear_svm.hpp>
#include <toolkits/supervised_learning/boosted_trees.hpp>
#include <toolkits/supervised_learning/random_forest.hpp>
#include <unity/lib/simple_model.hpp>
#include <unity/lib/toolkit_function_macros.hpp>

namespace turi {

static bool dml_class_inited = false;

void init_dml_class_registry() {
  if (!dml_class_inited) {
    dml_class_inited = true;
    auto& class_registry = dml_class_registry::get_instance();
    class_registry.register_model<simple_model>();
    class_registry.register_model<supervised::linear_regression>();
    class_registry.register_model<supervised::logistic_regression>();
    class_registry.register_model<supervised::linear_svm>();
    class_registry.register_model<supervised::xgboost::boosted_trees_regression>();
    class_registry.register_model<supervised::xgboost::boosted_trees_classifier>();
    class_registry.register_model<supervised::xgboost::random_forest_regression>();
    class_registry.register_model<supervised::xgboost::random_forest_classifier>();
  }
}


} // end of turicreate
