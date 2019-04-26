/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_XGBOOST_ITERATOR_H_
#define TURI_XGBOOST_ITERATOR_H_

#include <ml_data/ml_data.hpp>
#include <xgboost/src/learner/dmatrix.h>
#include <unity/toolkits/supervised_learning/xgboost.hpp>

namespace turi {
namespace supervised {
namespace xgboost {

class DMatrixMLData : public ::xgboost::learner::DMatrix {
 public: 
   DMatrixMLData(const ml_data &data, 
                 flexible_type class_weights = flex_undefined(),
                 storage_mode_enum storage_mode = storage_mode_enum::AUTO,
                 size_t max_row_per_batch = 0);

   virtual ~DMatrixMLData(void);
   virtual ::xgboost::IFMatrix *fmat(void) const;
   inline size_t num_classes(void) const { return num_classes_; }

 public:
   bool use_extern_memory_ = false;

 private:
   ::xgboost::IFMatrix *fmat_;
   size_t num_classes_ = 0;

   // not important here
   static const int kMagic = 0xffffab00;
};

}  // namespace xgboost
}  // namespace supervised
}  // namespace turi
#endif
