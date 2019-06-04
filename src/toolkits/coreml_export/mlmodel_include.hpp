/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef COREML_MLMODEL_HPP
#define COREML_MLMODEL_HPP

#if defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wreturn-type"
  #pragma GCC diagnostic ignored "-Wunused-function"
#elif defined (__clang__)
  #pragma clang diagnostic push
  #pragma clang diagnostic ignored "-Wsign-conversion"
  #pragma clang diagnostic ignored "-Wunused-parameter"
  #pragma clang diagnostic ignored "-Wshorten-64-to-32"
  #pragma clang diagnostic ignored "-Wshadow"
  #pragma clang diagnostic ignored "-Wextended-offsetof"
#endif

#pragma push_macro("CHECK")
#undef CHECK

// Include this first.  We need to undefine some defines here.
#include <mlmodel/src/transforms/TreeEnsemble.hpp>
#include <mlmodel/src/transforms/Pipeline.hpp>

#include <mlmodel/src/transforms/OneHotEncoder.hpp>
#include <mlmodel/src/transforms/FeatureVectorizer.hpp>
#include <mlmodel/src/transforms/DictVectorizer.hpp>
#include <mlmodel/src/transforms/LinearModel.hpp>



#ifdef MAX
#undef MAX
#endif

#ifdef MIN
#undef MIN
#endif

#pragma pop_macro("CHECK")

#if defined(__GNUC__)
  #pragma GCC diagnostic pop
#elif defined (__clang__)
  #pragma clang diagnostic pop
#endif

#endif


