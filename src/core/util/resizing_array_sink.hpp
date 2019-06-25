/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_RESIZING_COUNTING_SINK
#define TURI_RESIZING_COUNTING_SINK

#include <core/util/charstream.hpp>

namespace turi {

  typedef charstream_impl::resizing_array_sink<false> resizing_array_sink;

  /**
  Wraps a resizing array sink.
  */
  class resizing_array_sink_ref {
   private:
    resizing_array_sink* ras;
   public:


    typedef resizing_array_sink::char_type char_type;
    typedef resizing_array_sink::category category;

    inline resizing_array_sink_ref(resizing_array_sink& ref): ras(&ref) { }

    inline resizing_array_sink_ref(const resizing_array_sink_ref& other) :
      ras(other.ras) { }

    inline size_t size() const { return ras->size(); }
    inline char* c_str() { return ras->c_str(); }

    inline void clear() { ras->clear(); }
    /** the optimal buffer size is 0. */
    inline std::streamsize optimal_buffer_size() const {
      return ras->optimal_buffer_size();
    }

    inline void relinquish() { ras->relinquish(); }

    inline void advance(std::streamsize n) { ras->advance(n); }


    inline std::streamsize write(const char* s, std::streamsize n) {
      return ras->write(s, n);
    }
  };

}
#endif
