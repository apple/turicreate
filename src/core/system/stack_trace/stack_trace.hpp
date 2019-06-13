/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#ifndef TURI_STACK_TRACE_H_
#define TURI_STACK_TRACE_H_

#include <util/basic_types.hpp>

using std::string;
using std::ostream;
using std::vector;

#if defined(TC_HAS_LIBUNWIND) && defined(TC_HAS_LLVM)

#define UNW_LOCAL_ONLY
#include <libunwind.h>
#include <cxxabi.h>
#include <dlfcn.h>

struct stack_trace {
  vector<int64_t> addrs_;
};

struct line_info_debug {
  optional<string> sym_name_;
  optional<string> file_name_;
  optional<int64_t> line_;
};

struct stack_frame_debug {
  int64_t depth_;
  string library_name_;
  int64_t offset_;
  line_info_debug line_info_;
};

struct stack_trace_debug {
  vector<stack_frame_debug> frames_;
};

constexpr int64_t TC_STACK_TRACE_MAX_DEPTH_DEFAULT = 256;

void fill_stack_trace(stack_trace& trace, int64_t depth_max);
void fill_stack_trace(stack_trace& trace);

vector<line_info_debug> symbolize_addr_info(string dl_name, int64_t offset);

stack_trace_debug stack_trace_annotate(const stack_trace& x);

ostream& operator<<(ostream& os, const line_info_debug& x);
ostream& operator<<(ostream& os, const stack_trace_debug& x);
ostream& operator<<(ostream& os, const stack_trace& x_raw);

void write_annotated_stack_trace(ostream& os);

#endif // TC_HAS_LIBUNWIND && TC_HAS_LLVM

void write_annotated_stack_trace_if_configured(ostream& os);

#endif
