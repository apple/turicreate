/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */

#include <stack_trace/stack_trace.hpp>

#include <stack_trace/llvm_lib.hpp>
#include <util/basic_types.hpp>
#include <util/fs_util.hpp>
#include <util/string_util.hpp>

using std::max;
using std::vector;

#if defined(TC_HAS_LIBUNWIND) && defined(TC_HAS_LLVM)

void fill_stack_trace(stack_trace& trace, int64_t depth_max) {
  if (depth_max <= 0) {
    return;
  }

  unw_context_t ctx;
  unw_getcontext(&ctx);

  unw_cursor_t csr;
  int64_t ret = unw_init_local(&csr, &ctx);

  if (ret != 0) {
    cerr << "Error running unw_init_local." << endl;
    cerr << "Return code: " << ret;

    if (ret == UNW_EINVAL) {
      cerr << " (UNW_EINVAL)";
    } else if (ret == UNW_EBADREG) {
      cerr << " (UNW_EBADREG)";
    } else if (ret == UNW_EUNSPEC) {
      cerr << " (UNW_EUNSPEC)";
    }

    cerr << endl;
    return;
  }

  int64_t depth = 0;

  while (unw_step(&csr) > 0 && depth < depth_max) {
    unw_word_t pc;
    unw_get_reg(&csr, UNW_REG_IP, &pc);
    trace.addrs_.push_back(static_cast<int64_t>(pc));
    ++depth;
  }
}


void fill_stack_trace(stack_trace& trace) {
  fill_stack_trace(trace, TC_STACK_TRACE_MAX_DEPTH_DEFAULT);
}


vector<line_info_debug> symbolize_addr_info(string dl_name, int64_t offset) {
  using namespace llvm;
  using namespace symbolize;

  auto& symbolizer = get_llvm_symbolizer();

  auto res_or_err = symbolizer.symbolizeInlinedCode(dl_name, offset);

  vector<line_info_debug> ret;

  if (!res_or_err) {
    return ret;
  }

  auto res = res_or_err.get();

  int64_t num_frames = res.getNumberOfFrames();

  for (int64_t frame_index = 0; frame_index < num_frames; frame_index++) {
    DILineInfo curr_line_info = res.getFrame(frame_index);
    line_info_debug curr_line_info_debug;

    string sym_name_raw = curr_line_info.FunctionName;

    if (sym_name_raw.size() > 0 && sym_name_raw != "<invalid>") {
      string sym_name = sym_name_raw;
      auto paren_index = sym_name.find("(");
      if (paren_index != string::npos) {
        sym_name = sym_name.substr(0, paren_index);
        sym_name = rstrip_all(sym_name, " ");
      }

      string file_name = curr_line_info.FileName;

      if (file_name.size() > 0 && file_name != "<invalid>") {
        curr_line_info_debug.file_name_ = SOME(file_name);
      } else {
        curr_line_info_debug.file_name_ = NONE<string>();
      }

      curr_line_info_debug.sym_name_ = SOME(sym_name);

      if (!!curr_line_info_debug.file_name_) {
        curr_line_info_debug.line_ = SOME(static_cast<int64_t>(curr_line_info.Line));
      }
    }

    ret.push_back(curr_line_info_debug);
  }

  return ret;
}


stack_trace_debug stack_trace_annotate(const stack_trace& x) {
  stack_trace_debug ret;

  using namespace llvm;
  using namespace symbolize;

  for (int64_t depth = 0; depth < len(x.addrs_); depth++) {
    int64_t pc = x.addrs_[depth];

    optional<string> dl_name = NONE<string>();
    optional<int64_t> dl_fbase = NONE<int64_t>();

    Dl_info info;
    memset(&info, 0, sizeof(info));
    dladdr(reinterpret_cast<const void*>(pc), &info);
    if (!!info.dli_fname) {
      dl_name = SOME<string>(info.dli_fname);
      dl_fbase = SOME<int64_t>(int64_t(info.dli_fbase));
    }

    int64_t offset = int64_t(pc);

    if (!!dl_name) {
#if defined(TC_PLATFORM_LINUX)
      if (ends_with(*dl_name, ".so") || contains(*dl_name, ".so.")) {
        int64_t base_addr = int64_t(info.dli_fbase);
        offset -= base_addr;
      }
#elif defined(TC_PLATFORM_APPLE)
      int64_t base_addr = int64_t(*dl_fbase);

      offset -= base_addr;

      // TODO: are the following heuristics sufficient to handle Mach-O format?
      if (!ends_with(*dl_name, ".so") && !ends_with(*dl_name, ".dylib") &&
          !contains(*dl_name, ".so.") && !contains(*dl_name, ".dylib.") &&
          !ends_with(*dl_name, "Python")) {
        offset |= 0x100000000LL;
      }
#else
#error Unsupported platform.
#endif

      offset -= 1LL;

      vector<line_info_debug> line_info_seq = symbolize_addr_info(*dl_name, offset);
      vector<stack_frame_debug> curr_frames;

      for (auto line_info : line_info_seq) {
        stack_frame_debug frame;
        frame.depth_ = depth;
        string dl_name_abbrev = turi::fs_util::relativize_path(*dl_name, TC_BUILD_PATH_BASE);
        frame.library_name_ = dl_name_abbrev;
        frame.offset_ = offset;
        frame.line_info_ = line_info;
        if (!!frame.line_info_.file_name_) {
          frame.line_info_.file_name_ = SOME(
            turi::fs_util::relativize_path(*frame.line_info_.file_name_, TC_BUILD_PATH_BASE));
        }
        ret.frames_.push_back(frame);
        curr_frames.push_back(frame);
      }
    }
  }

  return ret;
}


ostream& operator<<(ostream& os, const line_info_debug& x) {
  if (!!x.sym_name_) {
    os << *x.sym_name_;
  } else {
    os << "??";
  }
  if (!!x.file_name_) {
    os << " (";
    os << *x.file_name_;
    os << ":";
    if (!!x.line_) {
      os << *x.line_;
    } else {
      os << "??";
    }
    os << ")";
  }
  return os;
}


ostream& operator<<(ostream& os, const stack_trace_debug& x) {
  vector<stack_frame_debug> frames_filtered;
  for (stack_frame_debug frame : x.frames_) {
    bool retain = true;
    if (!!frame.line_info_.sym_name_) {
      auto sym_name = *frame.line_info_.sym_name_;
      if (sym_name == "write_annotated_stack_trace" ||
          sym_name == "write_annotated_stack_trace_if_configured") {
        retain = false;
      }
    }

    if (retain) {
      frames_filtered.push_back(frame);
    }
  }

  if (len(frames_filtered) == 0) {
    fmt(os, "Stack trace (0 entries).\n");
    return os;
  }

  fmt(os, "Stack trace (%v entries):\n", len(frames_filtered));

  for (int64_t i = 0; i < len(frames_filtered); i++) {
    auto frame = frames_filtered[i];
    auto i_str = cc_sprintf("#%lld", i);
    auto i_pre = cc_repstr(" ", max<int64_t>(0, 5 - len(i_str)));
    fmt(os, "%v%v: %v", i_pre, i_str, frame.line_info_);
    if (!frame.line_info_.line_) {
      fmt(os, " [%v + %v]", frame.library_name_, cc_sprintf("0x%llx", frame.offset_));
    }
    os << endl;
  }
  return os;
}


ostream& operator<<(ostream& os, const stack_trace& x_raw) {
  stack_trace_debug x = stack_trace_annotate(x_raw);
  os << x;
  return os;
}


void write_annotated_stack_trace(ostream& os) {
  stack_trace trace;
  fill_stack_trace(trace);
  os << trace << endl;
}

#endif // TC_HAS_LIBUNWIND && TC_HAS_LLVM


void write_annotated_stack_trace_if_configured(ostream& os) {
#ifdef TC_STACK_DISPLAY
  write_annotated_stack_trace(os);
#endif
}
