/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SERIALIZE_GL_STRING_HPP
#define TURI_SERIALIZE_GL_STRING_HPP

#include <core/storage/serialization/iarchive.hpp>
#include <core/storage/serialization/oarchive.hpp>
#include <core/storage/serialization/iterator.hpp>
#include <core/generics/gl_string.hpp>

namespace turi {

  namespace archive_detail {

    /// Serialization of gl_string
    template <typename OutArcType>
    struct serialize_impl<OutArcType, gl_string, false> {
      static void exec(OutArcType& oarc, const gl_string& s) {
        size_t length = s.length();
        oarc << length;
        if(length > 0) {
          oarc.write(reinterpret_cast<const char*>(s.data()), (std::streamsize)length);
        }
        DASSERT_FALSE(oarc.fail());
      }
    };


    /// Deserialization of gl_string
    template <typename InArcType>
    struct deserialize_impl<InArcType, gl_string, false> {
      static void exec(InArcType& iarc, gl_string& s) {
        //read the length
        size_t length;
        iarc >> length;
        //resize the string and read the characters
        s.resize(length);
        if(length > 0) {
          iarc.read(&(s[0]), (std::streamsize)length);
        }
        DASSERT_FALSE(iarc.fail());
      }
    };
  }

} // namespace turi

#endif
