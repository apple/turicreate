/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// This file should not be included directly. use serialize.hpp
#ifndef TURI_SERIALIZE_HPP
#include <core/storage/serialization/serialize.hpp>

#else

#ifndef TURI_OARCHIVE_HPP
#define TURI_OARCHIVE_HPP

#include <iostream>
#include <string>
#include <core/logging/assertions.hpp>
#include <core/storage/serialization/is_pod.hpp>
#include <core/storage/serialization/has_save.hpp>
#include <core/util/branch_hints.hpp>
namespace turi {
  class dir_archive;
  /**
   * \ingroup group_serialization
   * \brief The serialization output archive object which, provided
   * with a reference to an ostream, will write to the ostream,
   * providing serialization capabilities.
   *
   * Given a standard output stream, you can construct an oarchive
   * object by:
   * \code
   *   // where strm is an ostream object
   *   turi::oarchive oarc(strm);
   * \endcode
   *
   * For instance, to serialize to a file,
   * \code
   *   std::ofstream fout("outputfile.bin");
   *   turi::oarchive oarc(fout);
   * \endcode
   *
   * Once the oarc object is constructed, \ref sec_serializable objects can be
   * written to it using the << stream operator.
   *
   * \code
   *    oarc << a << b << c;
   * \endcode
   *
   * Alternatively, data can be directly written to the stream
   * using the oarchive::write() function.
   *
   * Written data can be deserialized using turi::iarchive.
   * For more usage details, see \ref serialization
   *
   * The oarchive object should not be used once the associated stream
   * object is closed or is destroyed.
   *
   * The oarc object
   * does <b> not </b> flush the associated stream, and the user may need to
   * manually flush the associated stream to clear any stream buffers.
   * For instance, while the std::stringstream may be used for both output
   * and input, it is necessary to flush the stream before all bytes written to
   * the stringstream are available for input.
   *
   * If the oarchive is constructed without an ostream, writes go into a
   * newly allocated buffer in oarc.buf. The serialized length is oarc.off
   * and the entire buffer allocated length is oarc.len.
   *
   * If the oarchive is constructed with a std::vector<char>&, it will behave
   * as like the above case, but it will keep a reference to the
   * std::vector<char> and use that for resizing. At completion,
   * the std::vector<char> will contain the data and oarc.off contains the
   * serialized length.
   *
   * and the entire buffer allocated length is oarc.len.
   *
   * To use this class, include
   * core/storage/serialization/serialization_includes.hpp
   */
  class oarchive{
  public:
    std::ostream* out = NULL;
    dir_archive* dir = NULL;
    std::vector<char>* vchar = NULL; // if set, buf is a pointer into vchar
    char* buf = NULL;
    size_t off = 0;
    size_t len = 0;
    /// constructor. Takes a generic std::ostream object
    inline oarchive(std::ostream& outstream)
      : out(&outstream) {}

    inline oarchive(void) {}

    inline oarchive(std::vector<char>& vec) {
      vchar = &vec;
      buf = vchar->data();
      off = 0;
      len = vchar->size();
    }

    inline oarchive(dir_archive& dirarc)
      : out(dirarc.get_output_stream()),dir(&dirarc) {}

    inline void expand_buf(size_t s) {
        if (__unlikely__(off + s > len)) {
          len = 2 * (s + len);
          if (vchar != NULL) {
            vchar->resize(len);
            buf = vchar->data();
          } else {
            buf = (char*)realloc(buf, len);
          }
        }
     }
    /** Directly writes "s" bytes from the memory location
     * pointed to by "c" into the stream.
     */
    inline void write(const char* c, std::streamsize s) {
      if (out == NULL) {
        expand_buf(s);
        memcpy(buf + off, c, s);
        off += s;
      } else {
        out->write(c, s);
      }
    }
    template <typename T>
    inline void direct_assign(const T& t) {
      if (out == NULL) {
        expand_buf(sizeof(T));
        std::memcpy(buf + off, &t, sizeof(T));
        off += sizeof(T);
      }
      else {
        out->write(reinterpret_cast<const char*>(&t), sizeof(T));
      }
    }

    inline void advance(size_t s) {
      if (out == NULL) {
        expand_buf(s);
        off += s;
      } else {
        out->seekp(s, std::ios_base::cur);
      }
    }

    /// Returns true if the underlying stream is in a failure state
    inline bool fail() {
      return out == NULL ? false : out->fail();
    }

    std::string get_prefix() {
      ASSERT_NE(dir, NULL);
      return dir->get_next_write_prefix();
    }

    inline ~oarchive() { }
  };

  /**
   * \ingroup group_serialization
   * \brief
   * When this archive is used to serialize an object,
   * and the object does not support serialization,
   * failure will only occur at runtime. Otherwise equivalent to
   * turi::oarchive
   */
  class oarchive_soft_fail {
  public:
    oarchive* oarc;
    bool mine;

    /// constructor. Takes a generic std::ostream object
    inline oarchive_soft_fail(std::ostream& outstream)
      : oarc(new oarchive(outstream)), mine(true) { }

    inline oarchive_soft_fail(oarchive& oarc):oarc(&oarc), mine(false) {
    }

    inline oarchive_soft_fail(void)
      : oarc(new oarchive) {}

    /** Directly writes "s" bytes from the memory location
     * pointed to by "c" into the stream.
     */
    inline void write(const char* c, std::streamsize s) {
      oarc->write(c, s);
    }
    template <typename T>
    inline void direct_assign(const T& t) {
      oarc->direct_assign(t);
    }

    inline bool fail() {
      return oarc->fail();
    }

    std::string get_prefix() {
      return oarc->get_prefix();
    }

    inline ~oarchive_soft_fail() {
     if (mine) delete oarc;
    }
  };

  namespace archive_detail {

    /// called by the regular archive The regular archive will do a hard fail
    template <typename OutArcType, typename T>
    struct serialize_hard_or_soft_fail {
      inline static void exec(OutArcType& oarc, const T& t) {
        t.save(oarc);
      }
    };

    /// called by the soft fail archive
    template <typename T>
    struct serialize_hard_or_soft_fail<oarchive_soft_fail, T> {
      inline static void exec(oarchive_soft_fail& oarc, const T& t) {
        // create a regular oarchive and
        // use the save_or_fail function which will
        // perform a soft fail
        save_or_fail(*(oarc.oarc), t);
      }
    };


    /**
       Implementation of the serializer for different types.
       This is the catch-all. If it gets here, it must be a non-POD and is a class.
       We therefore call the .save function.
       Here we pick between the archive types using serialize_hard_or_soft_fail
    */
    template <typename OutArcType, typename T, bool IsPOD, typename Enable = void>
    struct serialize_impl {
      static void exec(OutArcType& oarc, const T& t) {
        serialize_hard_or_soft_fail<OutArcType, T>::exec(oarc, t);
      }
    };

    /** Catch if type is a POD */
    template <typename OutArcType, typename T>
    struct serialize_impl<OutArcType, T, true> {
      inline static void exec(OutArcType& oarc, const T& t) {
        oarc.direct_assign(t);
        //oarc.write(reinterpret_cast<const char*>(&t), sizeof(T));
      }
    };

    /**
       Re-dispatch if for some reasons T already has a const
    */
    template <typename OutArcType, typename T>
    struct serialize_impl<OutArcType, const T, true> {
      inline static void exec(OutArcType& oarc, const T& t) {
        serialize_impl<OutArcType, T, true>::exec(oarc, t);
      }
    };

    /**
       Re-dispatch if for some reasons T already has a const
    */
    template <typename OutArcType, typename T>
    struct serialize_impl<OutArcType, const T, false> {
      inline static void exec(OutArcType& oarc, const T& t) {
        serialize_impl<OutArcType, T, false>::exec(oarc, t);
      }
    };
  }// archive_detail


  /// \cond TURI_INTERNAL

  /**
     Overloads the operator<< in the oarchive to
     allow the use of the stream syntax for serialization.
     It simply re-dispatches into the serialize_impl classes
  */
  template <typename T>
  inline oarchive& operator<<(oarchive& oarc, const T& t) {
    archive_detail::serialize_impl<oarchive,
                                   T,
                                   gl_is_pod<T>::value >::exec(oarc, t);
    return oarc;
  }

  /**
     Overloads the operator<< in the oarchive_soft_fail to
     allow the use of the stream syntax for serialization.
     It simply re-dispatches into the serialize_impl classes
  */
  template <typename T>
  inline oarchive_soft_fail& operator<<(oarchive_soft_fail& oarc,
                                        const T& t) {
    archive_detail::serialize_impl<oarchive_soft_fail,
                                  T,
                                  gl_is_pod<T>::value >::exec(oarc, t);
    return oarc;
  }


  /**
     Serializes an arbitrary pointer + length to an archive
  */
  inline oarchive& serialize(oarchive& oarc,
                             const void* str,
                             const size_t length) {
    // save the length
    oarc.write(reinterpret_cast<const char*>(str),
                    (std::streamsize)length);
    assert(!oarc.fail());
    return oarc;
  }


  /**
     Serializes an arbitrary pointer + length to an archive
  */
  inline oarchive_soft_fail& serialize(oarchive_soft_fail& oarc,
                                       const void* str,
                                       const size_t length) {
    // save the length
    oarc.write(reinterpret_cast<const char*>(str),
                    (std::streamsize)length);
    assert(!oarc.fail());
    return oarc;
  }

  /// \endcond TURI_INTERNAL

}
  /**
     \ingroup group_serialization
     \brief Macro to make it easy to define out-of-place saves

     In the event that it is impractical to implement a save() and load()
     function in the class one wnats to serialize, it is necessary to define
     an "out of save" save and load.

     See \ref sec_serializable_out_of_place for an example

     \note important! this must be defined in the global namespace!
  */
#define BEGIN_OUT_OF_PLACE_SAVE(arc, tname, tval)                       \
  namespace turi{ namespace archive_detail {                        \
  template <typename OutArcType> struct serialize_impl<OutArcType, tname, false> { \
  static void exec(OutArcType& arc, const tname & tval) {

#define END_OUT_OF_PLACE_SAVE() } }; } }


#endif

#endif
