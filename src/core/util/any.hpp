/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
// Modified from boost 1.37 boost::any
// Extended to handle turicreate serialization/deserialization functions
// See http://www.boost.org/libs/any for Documentation.

#ifndef TURI_ANY_INCLUDED
#define TURI_ANY_INCLUDED

// what:  variant type boost::any
// who:   contributed by Kevlin Henney,
//        with features contributed and bugs found by
//        Ed Brey, Mark Rodgers, Peter Dimov, and James Curran
// when:  July 2001
// where: tested with BCC 5.5, MSVC 6.0, and g++ 2.95

#include <algorithm>
#include <typeinfo>
#include <map>
#include <iostream>
#include <stdint.h>

#include <boost/type_traits/remove_reference.hpp>
#include <boost/type_traits/is_reference.hpp>
#include <boost/throw_exception.hpp>
#include <boost/static_assert.hpp>
#include <boost/utility.hpp>
#include <boost/exception/detail/is_output_streamable.hpp>
#include <boost/functional/hash.hpp>


#include <core/logging/assertions.hpp>
#include <core/storage/serialization/serialization_includes.hpp>

namespace turi {
  /**
   * \ingroup util
   A generic "variant" object obtained from Boost::Any and modified to
   be serializable. A variable of type "any" can store any datatype
   (even dynamically changeable at runtime), but the caveat is that
   you must know the exact stored type to be able to extract the data
   safely.

   To serialize/deserialize the any, regular serialization procedures
   apply.  However, since a statically initialized type registration
   system is used to identify the type of the deserialized object, so
   the user must pay attention to a couple of minor issues.

   On serialization:

   \li \b a) If an any contains a serializable type, the any can be
             serialized.
   \li \b b) If an any contains an unserializable type, the
             serialization will fail at runtime.

   On deserialization:

   \li \b c) An empty any can be constructed with no type information
             and it can be deserialized from an archive.
   \li \b d) However, the deserialization will fail at runtime if the
             true type of the any is never accessed / instantiated
             anywhere in the code.

   Condition \b d) is particular unusual so I will illustrate with an
   example.

   Given a simple user struct:
   \code
   struct UserStruct {
     int i;
     void save (turi::oarchive& oarc) const {
       oarc << i;
     }
     void load (turi::iarchive& iarc) {
       iarc << i;
     }
   }
  \endcode

   If an any object contains the struct, it will be serializable.

   \code
   UserStruct us;
   us.i = 10;
   any a = us;
   // output file
   std::ofstream fout("test.bin");
   turi::oarchive oarc(fout);
   oarc << a;    // write the any
   \endcode

   To deserialize, I will open an input archive and stream into an any.

   \code
   // open input
   std::ifstream fin("test.bin");
   turi::iarchive iarc(fin);
   // create an any and read it
   any a;
   iarc >> a;
   \endcode

   Now, unusually, the above code will fail, while the following code
   will succeed

   \code
   // open input
   std::ifstream fin("test.bin");
   turi::iarchive iarc(fin);
   // create an any and read it
   any a;
   iarc >> a;
   std::cout << a.as<UserStruct>().i;
   \endcode

   The <tt> a.as<UserStruct>() </tt> forces the instantiation of static functions
   which allow the any deserialization to identify the UserStruct type.
  */
  class any {
  private:
    /**
     * iholder is the base abstract type used to store the contents
     */
    class iholder {
    public: // structors
      virtual ~iholder() { }
      virtual const std::type_info& type() const = 0;
      virtual iholder * clone() const = 0;
      virtual uint64_t deserializer_id() const = 0;
      virtual void deep_op_equal(const iholder* c) = 0;
      static iholder* load(iarchive_soft_fail &arc);
      virtual void save(oarchive_soft_fail& arc) const = 0;
      virtual std::ostream& print(std::ostream& out) const = 0;
    };
    iholder* contents;

  public: // structors
    /// default constructor. Creates an empty any
    any() : contents(NULL) { }

    /// Creates an any which stores the value
    template<typename ValueType>
    explicit any(const ValueType& value)
      : contents(new holder<ValueType>(value)) { }

    /// Construct an any from another any
    any(const any & other) :
      contents(other.empty() ? NULL : other.contents->clone()) { }

    /// Destroy the contentss of this any
    ~any() { delete contents; }

    /// Returns true if the object does not contain any stored data
    bool empty() const { return contents == NULL; }

    /// Extracts a reference to the contents of the any as a type of
    /// ValueType
    template<typename ValueType>
    ValueType& as() {
      DASSERT_TRUE(type() == typeid(ValueType));
      DASSERT_FALSE(empty());
      return static_cast<holder<ValueType> *>(contents)->contents;
    }

    /// Extracts a constant reference to the contents of the any as a
    /// type of ValueType
    template<typename ValueType>
    inline const ValueType& as() const{
      DASSERT_TRUE(type() == typeid(ValueType));
      DASSERT_FALSE(empty());
      return static_cast< holder<ValueType> *>(contents)->contents;
    }

    /// Returns true if the contained type is of type ValueType
    template<typename ValueType>
    bool is() const{
      return (type() == typeid(ValueType));
    }


    /// Exchanges the contents of two any's
    any& swap(any & rhs) {
      std::swap(contents, rhs.contents);
      return *this;
    }

    /**
     * Update the contents of this any.  If a new type is used than
     * the type of this any will change.
     */
    template<typename ValueType>
    any& operator=(const ValueType & rhs) {
      if (contents != NULL && contents->type() == typeid(ValueType)) {
        as<ValueType>() = rhs;
      } else { any(rhs).swap(*this); }
      return *this;
    }

    /**
     * Update the contents of this any to match the type of the other
     * any.
     */
    any& operator=(const any & rhs) {
      if (rhs.empty()) {
        if (contents) delete contents;
        contents = NULL;
      } else {
        if (contents != NULL && contents->type() == rhs.contents->type()) {
          contents->deep_op_equal(rhs.contents);
        } else { any(rhs).swap(*this); }
      }
      return *this;
    }

    std::ostream& print(std::ostream& out) const {
      return empty()? (out << "EMPTY") : contents->print(out);
    }

    /// Returns the type information of the stored data.
    const std::type_info& type() const {
      return empty() ? typeid(void) : contents->type();
    }

    /// Return the name of the internal type as a string.
    const std::string type_name() const {
      return empty() ? "NULL" : std::string(contents->type().name());
    }

    /// loads the any from a file.
    void load(iarchive& arc) {
      iarchive_soft_fail isoftarc(arc);
      if(contents != NULL) { delete contents; contents = NULL; }
      bool isempty(true);
      isoftarc >> isempty;
      if (isempty == false) contents = iholder::load(isoftarc);
    }

    /// Saves the any to a file. Caveats apply. See the main any docs.
    void save(oarchive& arc) const {
      oarchive_soft_fail osoftarc(arc);
      bool isempty = empty();
      osoftarc << isempty;
      if (isempty == false) contents->save(osoftarc);
    }


  public:
    /**
     * This section contain the global registry used to determine the
     * deserialization code for a particular type.  Essentially the
     * registry is a global map in which all subtypes of iholder
     * register a deserialization function with their type.
     */

    typedef iholder* (*deserialize_function_type)(iarchive_soft_fail& arc);
    typedef std::map<uint64_t, deserialize_function_type> registry_map_type;
    /**
     * The get registry routine is a static method that gets a
     * reference to the global registry.  It is very important that
     * this be a static method and not a static member to ensure that
     * the global registry is defined before each holders try to
     * register.  This is accomplished by having get_registry
     * statically declare the global registry
     */
    static registry_map_type& get_global_registry();

  public:

    template <typename ValueType> static
    typename boost::disable_if_c<boost::is_output_streamable<ValueType>::value,
                                 void>::type
    print_type_or_contents(std::ostream& out, const ValueType &h) {
      out << "Not_Printable[" << typeid(ValueType).name() << ']';
    }

    template <typename ValueType> static
    typename boost::enable_if_c<boost::is_output_streamable<ValueType>::value,
                                void>::type
    print_type_or_contents(std::ostream& out, const ValueType &h) { out << h; }


  public:

    /**
     * holder is an instantiation of iholder
     */
    template<typename ValueType>
    class holder : public iholder {
    public:
      typedef ValueType value_type;
      /// The actual contents of the holder
      ValueType contents;
      /// Construct a holder from a value
      holder(const ValueType& value) : contents(value) { }
      /// Construct a holder from an archive
      holder(iarchive_soft_fail& arc) { arc >> contents; }
      /// Get the type info of the holder
      const std::type_info& type() const { return typeid(ValueType); }
      /// Clone a holder
      iholder* clone() const { return new holder(contents); }
      /// Deep assignment
      void deep_op_equal(const iholder* other) {
        contents = static_cast< const holder<ValueType>* >(other)->contents;
      }
      /**
       * Get the deserializer id from the static registry associated
       * with this type of holder
       */
      uint64_t deserializer_id() const { return registry.localid; }
      void save(oarchive_soft_fail &arc) const {
        arc << registry.localid << contents;
      }
      /**
       * Print the contents or the type if the contents does not
       * support printing
       */
      std::ostream& print(std::ostream& out) const {
        any::print_type_or_contents(out, contents);
        return out;
      }
      /** The actual deserialization function for this holder type */
      static iholder* deserialize(iarchive_soft_fail &arc) {
        return new holder(arc);
      }
      /**
       * The following struct defines the static member used to
       * automatically register the deserialization function for this
       * holder type and cache a shared id used to quickly identify
       * the deserialization function.
       *
       * Note that the registry actually uses the NAME of the type so
       * renaming a type will result in an incompatible
       * deserialization.
       */
      struct registry_type {
        uint64_t localid;
        registry_type() {
          boost::hash<std::string> hash_function;
          // compute localid
          localid = hash_function(typeid(ValueType).name());
          any::get_global_registry()[localid] = holder::deserialize;
        }
      }; // end of registry type
      /**
       * The registry is a static member that will get constructed
       * before main and used to register the any type
       */
      static registry_type registry;
    private:
      holder& operator=(const holder& other) { }
    }; // end of class holder

  }; // end of class any


  /**
   * This static membery computes the holder (type specific)
   * deserialization id and also registers it with the global
   * registry.
   */
  template<typename ValueType>
  typename any::holder<ValueType>::registry_type any::holder<ValueType>::registry;

} // namespace turi

std::ostream& operator<<(std::ostream& out, const turi::any& any);


// Copyright Kevlin Henney, 2000, 2001, 2002. All rights reserved.
//
// Distributed under the Boost Software License, Version 1.0. (See
// accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt)

#endif
