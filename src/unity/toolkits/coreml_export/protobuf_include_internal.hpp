/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef _TURI_PROTOBUF_INCLUDE_INTERNAL
#define _TURI_PROTOBUF_INCLUDE_INTERNAL

#if defined(__GNUC__)
  #pragma GCC diagnostic push
  #pragma GCC diagnostic ignored "-Wreturn-type"
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

#include <google/protobuf/text_format.h>
#include <google/protobuf/io/coded_stream.h>
#include <google/protobuf/io/zero_copy_stream_impl.h>

#include "unity/toolkits/coreml_export/MLModel/build/format/FeatureTypes_enums.h"
#include "unity/toolkits/coreml_export/MLModel/build/format/Model_enums.h"
#include "unity/toolkits/coreml_export/MLModel/build/format/NeuralNetwork_enums.h"
#include "unity/toolkits/coreml_export/MLModel/build/format/OneHotEncoder_enums.h"

#include "unity/toolkits/coreml_export/MLModel/build/format/Model.pb.h"

#pragma pop_macro("CHECK")

#if defined(__GNUC__)
  #pragma GCC diagnostic pop
#elif defined (__clang__)
  #pragma clang diagnostic pop
#endif

#endif
