/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_SERIALIZE_HPP
#include <core/storage/serialization/serialize.hpp>

#else


#ifndef TURI_IARCHIVE_HPP
#define TURI_IARCHIVE_HPP

#include <iostream>
#include <core/logging/assertions.hpp>
#include <core/storage/serialization/is_pod.hpp>
#include <core/storage/serialization/has_load.hpp>
#include <core/storage/serialization/dir_archive.hpp>
namespace turi {

  /**
   * \ingroup group_serialization
   * \brief The serialization input archive object which, provided
   * with a reference to an istream, will read from the istream,
   * providing deserialization capabilities.
   *
   * Given a source of serialized bytes (written by an turi::oarchive),
   * in the form of a standard input stream, you can construct an iarchive
   * object by:
   * \code
   *   // where strm is an istream object
   *   turi::iarchive iarc(strm);
   * \endcode
   *
   * For instance, to deserialize from a file,
   * \code
   *   std::ifstream fin("inputfile.bin");
   *   turi::iarchive iarc(fin);
   * \endcode
   *
   * Once the iarc object is constructed, \ref sec_serializable
   * objects can be read from it using the >> stream operator.
   *
   * \code
   *    iarc >> a >> b >> c;
   * \endcode
   *
   * Alternatively, data can be directly read from the stream using
   * the iarchive::read() and iarchive::read_char() functions.
   *
   * For more usage details, see \ref serialization
   *
   * The iarchive object should not be used once the associated stream
   * object is closed or is destroyed.
   *
   * To use this class, include
   * core/storage/serialization/serialization_includes.hpp
   */
  class iarchive {
  public:
    std::istream* in = NULL;
    dir_archive* dir = NULL;
    const char* buf = NULL;
    size_t off = 0;
    size_t len = 0;


    /**
     * Constructs an iarchive object.
     * Takes a reference to a generic std::istream object and associates
     * the archive with it. Reads from the archive will read from the
     * assiciated input stream.
     */
    inline iarchive(std::istream& instream)
      : in(&instream) { }

    inline iarchive(const char* buf, size_t len)
      : buf(buf), off(0), len(len) { }


    inline iarchive(dir_archive& dirarc)
      : in(dirarc.get_input_stream()),dir(&dirarc) {}

    ~iarchive() {}

    /// Directly reads a single character from the input stream
    inline char read_char() {
      char c;
      if (buf) {
        c = buf[off];
        ++off;
      } else {
        in->get(c);
      }
      return c;
    }

    /**
     *  Directly reads a sequence of "len" bytes from the
     *  input stream into the location pointed to by "c"
     */
    inline void read(char* c, size_t l) {
      if (buf) {
        memcpy(c, buf + off, l);
        off += l;
      } else {
        in->read(c, l);
      }
    }


    /**
     *  Directly reads a sequence of "len" bytes from the
     *  input stream into the location pointed to by "c"
     */
    template <typename T>
    inline void read_into(T& c) {
      if (buf) {
        memcpy(&c, buf + off, sizeof(T));
        off += sizeof(T);
      } else {
        in->read(reinterpret_cast<char*>(&c), sizeof(T));
      }
    }

    /// Returns true if the underlying stream is in a failure state
    inline bool fail() {
      return in == NULL ? off > len : in->fail();
    }

    std::string get_prefix() {
      ASSERT_NE(dir, NULL);
      return dir->get_next_read_prefix();
    }
  };


  /**
   * \ingroup group_serialization
   * \brief
   * When this archive is used to deserialize an object,
   * and the object does not support serialization,
   * failure will only occur at runtime. Otherwise equivalent to
   * turi::iarchive.
   */
  class iarchive_soft_fail{
  public:

    iarchive *iarc;
    bool mine;

    /// Directly reads a single character from the input stream
    inline char read_char() {
      return iarc->read_char();
    }

    /**
     *  Directly reads a sequence of "len" bytes from the
     *  input stream into the location pointed to by "c"
     */
    inline void read(char* c, size_t len) {
      iarc->read(c, len);
    }

    /**
     *  Directly reads a sequence of "len" bytes from the
     *  input stream into the location pointed to by "c"
     */
    template <typename T>
    inline void read_into(T& c) {
      iarc->read_into(c);
    }


    /// Returns true if the underlying stream is in a failure state
    inline bool fail() {
      return iarc->fail();
    }


    std::string get_prefix() {
      return iarc->get_prefix();
    }

    /**
     * Constructs an iarchive_soft_fail object.
     * Takes a reference to a generic std::istream object and associates
     * the archive with it. Reads from the archive will read from the
     * assiciated input stream.
     */
    inline iarchive_soft_fail(std::istream &instream)
      : iarc(new iarchive(instream)), mine(true) {}

    /**
     * Constructs an iarchive_soft_fail object from an iarchive.
     * Both will share the same input stream
     */
    inline iarchive_soft_fail(iarchive &iarc)
      : iarc(&iarc), mine(false) {}

    inline ~iarchive_soft_fail() { if (mine) delete iarc; }
  };


  namespace archive_detail {

    /// called by the regular archive The regular archive will do a hard fail
    template <typename InArcType, typename T>
    struct deserialize_hard_or_soft_fail {
      inline static void exec(InArcType& iarc, T& t) {
        t.load(iarc);
      }
    };

    /// called by the soft fail archive
    template <typename T>
    struct deserialize_hard_or_soft_fail<iarchive_soft_fail, T> {
      inline static void exec(iarchive_soft_fail& iarc, T& t) {
        load_or_fail(*(iarc.iarc), t);
      }
    };


    /**
       Implementation of the deserializer for different types.  This is the
       catch-all. If it gets here, it must be a non-POD and is a class.  We
       therefore call the .save function.  Here we pick between the archive
       types using serialize_hard_or_soft_fail
    */
    template <typename InArcType, typename T, bool IsPOD, typename Enable = void>
    struct deserialize_impl {
      inline static void exec(InArcType& iarc, T& t) {
        deserialize_hard_or_soft_fail<InArcType, T>::exec(iarc, t);
      }
    };

    // catch if type is a POD
    template <typename InArcType, typename T>
    struct deserialize_impl<InArcType, T, true>{
      inline static void exec(InArcType& iarc, T &t) {
        iarc.read_into(t);
      }
    };

  } //namespace archive_detail

  /// \cond TURI_INTERNAL

  /**
     Allows Use of the "stream" syntax for serialization
  */
  template <typename T>
  inline iarchive& operator>>(iarchive& iarc, T &t) {
    archive_detail::deserialize_impl<iarchive,
                                     T,
                                     gl_is_pod<T>::value >::exec(iarc, t);
    return iarc;
  }



  /**
     Allows Use of the "stream" syntax for serialization
  */
  template <typename T>
  inline iarchive_soft_fail& operator>>(iarchive_soft_fail& iarc, T &t) {
    archive_detail::deserialize_impl<iarchive_soft_fail,
                                     T,
                                     gl_is_pod<T>::value >::exec(iarc, t);
    return iarc;
  }


  /**
     deserializes an arbitrary pointer + length from an archive
  */
  inline iarchive& deserialize(iarchive& iarc,
                               void* str,
                               const size_t length) {
    iarc.read(reinterpret_cast<char*>(str), (std::streamsize)length);
    assert(!iarc.fail());
    return iarc;
  }



  /**
     deserializes an arbitrary pointer + length from an archive
  */
  inline iarchive_soft_fail& deserialize(iarchive_soft_fail& iarc,
                                         void* str,
                                         const size_t length) {
    iarc.read(reinterpret_cast<char*>(str), (std::streamsize)length);
    assert(!iarc.fail());
    return iarc;
  }

  /// \endcond TURI_INTERNAL

  /**
     \ingroup group_serialization

     \brief Macro to make it easy to define out-of-place loads

     In the event that it is impractical to implement a save() and load()
     function in the class one wnats to serialize, it is necessary to define
     an "out of save" save and load.

     See \ref sec_serializable_out_of_place for an example

     \note important! this must be defined in the global namespace!
  */
#define BEGIN_OUT_OF_PLACE_LOAD(arc, tname, tval)       \
  namespace turi{ namespace archive_detail {        \
  template <typename InArcType>                           \
  struct deserialize_impl<InArcType, tname, false>{       \
  static void exec(InArcType& arc, tname & tval) {

#define END_OUT_OF_PLACE_LOAD() } }; } }




} // namespace turi


#endif

#endif
