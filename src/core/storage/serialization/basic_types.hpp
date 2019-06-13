/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
/*
   This files defines the serializer/deserializer for all basic types
   (as well as string and pair)
*/
#ifndef ARCHIVE_BASIC_TYPES_HPP
#define ARCHIVE_BASIC_TYPES_HPP

#include <string>
#include <core/storage/serialization/serializable_pod.hpp>
#include <core/logging/assertions.hpp>
#include <cstdint>

namespace turi {
  class oarchive;
  class iarchive;
}


namespace turi {
  namespace archive_detail {

    /** Serialization of null terminated const char* strings.
     * This is necessary to serialize constant strings like
     * \code
     * oarc << "hello world";
     * \endcode
     */
    template <typename OutArcType>
    struct serialize_impl<OutArcType, const char*, false> {
      static void exec(OutArcType& oarc, const char* const& s) {
        // save the length
        // ++ for the \0
        size_t length = strlen(s); length++;
        oarc << length;
        oarc.write(reinterpret_cast<const char*>(s), length);
        DASSERT_FALSE(oarc.fail());
      }
    };




    /// Serialization of fixed length char arrays
    template <typename OutArcType, size_t len>
    struct serialize_impl<OutArcType, char [len], false> {
      static void exec(OutArcType& oarc, const char s[len] ) {
        size_t length = len;
        oarc << length;
        oarc.write(reinterpret_cast<const char*>(s), length);
        DASSERT_FALSE(oarc.fail());
      }
    };


    /// Serialization of null terminated char* strings
    template <typename OutArcType>
    struct serialize_impl<OutArcType, char*, false> {
      static void exec(OutArcType& oarc, char* const& s) {
        // save the length
        // ++ for the \0
        size_t length = strlen(s); length++;
        oarc << length;
        oarc.write(reinterpret_cast<const char*>(s), length);
        DASSERT_FALSE(oarc.fail());
      }
    };

    /// Deserialization of null terminated char* strings
    template <typename InArcType>
    struct deserialize_impl<InArcType, char*, false> {
      static void exec(InArcType& iarc, char*& s) {
        // Save the length and check if lengths match
        size_t length;
        iarc >> length;
        s = new char[length];
        //operator>> the rest
        iarc.read(reinterpret_cast<char*>(s), length);
        DASSERT_FALSE(iarc.fail());
      }
    };

    /// Deserialization of fixed length char arrays
    template <typename InArcType, size_t len>
    struct deserialize_impl<InArcType, char [len], false> {
      static void exec(InArcType& iarc, char s[len]) {
        size_t length;
        iarc >> length;
        ASSERT_LE(length, len);
        iarc.read(reinterpret_cast<char*>(s), length);
        DASSERT_FALSE(iarc.fail());
      }
    };



    /// Serialization of std::string
    template <typename OutArcType>
    struct serialize_impl<OutArcType, std::string, false> {
      static void exec(OutArcType& oarc, const std::string& s) {
        size_t length = s.length();
        oarc << length;
        oarc.write(reinterpret_cast<const char*>(s.c_str()),
                   (std::streamsize)length);
        DASSERT_FALSE(oarc.fail());
      }
    };


    /// Deserialization of std::string
    template <typename InArcType>
    struct deserialize_impl<InArcType, std::string, false> {
      static void exec(InArcType& iarc, std::string& s) {
        //read the length
        size_t length;
        iarc >> length;
        //resize the string and read the characters
        s.resize(length);
        iarc.read(&(s[0]), (std::streamsize)length);
        DASSERT_FALSE(iarc.fail());
      }
    };

    /// Serialization of std::pair
    template <typename OutArcType, typename T, typename U>
    struct serialize_impl<OutArcType, std::pair<T, U>, false > {
      static void exec(OutArcType& oarc, const std::pair<T, U>& s) {
        oarc << s.first << s.second;
      }
    };


    /// Deserialization of std::pair
    template <typename InArcType, typename T, typename U>
    struct deserialize_impl<InArcType, std::pair<T, U>, false > {
      static void exec(InArcType& iarc, std::pair<T, U>& s) {
        iarc >> s.first >> s.second;
      }
    };


//
//     /** Serialization of 8 byte wide integers
//      * \code
//      * oarc << vec.length();
//      * \endcode
//      */
//     template <typename OutArcType>
//     struct serialize_impl<OutArcType, unsigned long , true> {
//       static void exec(OutArcType& oarc, const unsigned long & s) {
//         // only bottom 1 byte
//         if ((s >> 8) == 0) {
//           unsigned char c = 0;
//           unsigned char trunc_s = s;
//           oarc.direct_assign(c);
//           oarc.direct_assign(trunc_s);
//         }
//         // only bottom 2 byte
//         else if ((s >> 16) == 0) {
//           unsigned char c = 1;
//           unsigned short trunc_s = s;
//           oarc.direct_assign(c);
//           oarc.direct_assign(trunc_s);
//         }
//         // only bottom 4 byte
//         else if ((s >> 32) == 0) {
//           unsigned char c = 2;
//           uint32_t trunc_s = s;
//           oarc.direct_assign(c);
//           oarc.direct_assign(trunc_s);
//         }
//         else {
//           unsigned char c = 3;
//           oarc.direct_assign(c);
//           oarc.direct_assign(s);
//         }
//       }
//     };
//
//
//     /// Deserialization of 8 byte wide integer
//     template <typename InArcType>
//     struct deserialize_impl<InArcType, unsigned long , true> {
//       static void exec(InArcType& iarc, unsigned long & s) {
//         unsigned char c;
//         iarc.read(reinterpret_cast<char*>(&c), 1);
//         switch(c) {
//          case 0: {
//            unsigned char val;
//            iarc.read(reinterpret_cast<char*>(&val), 1);
//            s = val;
//            break;
//          }
//          case 1: {
//            unsigned short val;
//            iarc.read(reinterpret_cast<char*>(&val), 2);
//            s = val;
//            break;
//          }
//          case 2: {
//            uint32_t val;
//            iarc.read(reinterpret_cast<char*>(&val), 4);
//            s = val;
//            break;
//          }
//          case 3: {
//            iarc.read(reinterpret_cast<char*>(&s), 8);
//            break;
//          }
//          default:
//            ASSERT_LE(c, 3);
//         };
//       }
//     };
//


  } // namespace archive_detail
} // namespace turi

#undef INT_SERIALIZE
#endif
