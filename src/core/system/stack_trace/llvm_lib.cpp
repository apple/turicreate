/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <stack_trace/llvm_lib.hpp>

#include <util/basic_types.hpp>

using std::make_shared;
using std::shared_ptr;

#ifdef TC_HAS_LLVM

llvm::symbolize::LLVMSymbolizer& get_llvm_symbolizer() {
  using namespace llvm;
  using namespace symbolize;

  static thread_local shared_ptr<LLVMSymbolizer> symbolizer { nullptr };

  if (!symbolizer) {
    auto opts = LLVMSymbolizer::Options(FunctionNameKind::LinkageName, true, true, false, "");
    symbolizer = make_shared<LLVMSymbolizer>(opts);
  }

  return *symbolizer;
}

#endif // TC_HAS_LLVM
