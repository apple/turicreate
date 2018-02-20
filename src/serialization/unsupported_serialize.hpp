/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_UNSUPPORTED_SERIALIZE_HPP
#define TURI_UNSUPPORTED_SERIALIZE_HPP

#include <serialization/iarchive.hpp>
#include <serialization/oarchive.hpp>
#include <logger/logger.hpp>

namespace turi {

  /**
   * \ingroup group_serialization
   *  \brief Inheritting from this class will prevent the serialization
   *         of the derived class. Used for debugging purposes.
   * 
   *  Inheritting from this class will result in an assertion failure
   * if any attempt is made to serialize or deserialize the derived
   * class. This is largely used for debugging purposes to enforce
   * that certain types are never serialized 
   */
  struct unsupported_serialize {
    void save(oarchive& archive) const {      
      ASSERT_MSG(false, "trying to serialize an unserializable object");
    }
    void load(iarchive& archive) {
      ASSERT_MSG(false, "trying to deserialize an unserializable object");
    }
  }; // end of struct
};


/**
\ingroup group_serialization
\brief A macro which disables the serialization of type so that 
it will fault at runtime. 

Writing TURI_UNSERIALIZABLE(T) for some typename T in the global namespace
will result in an assertion failure if any attempt is made to serialize or
deserialize the type T.  This is largely used for debugging purposes to enforce
that certain types are never serialized. 
*/
#define TURI_UNSERIALIZABLE(tname) \
  BEGIN_OUT_OF_PLACE_LOAD(arc, tname, tval) \
    ASSERT_MSG(false, "trying to deserialize an unserializable object"); \
  END_OUT_OF_PLACE_LOAD()                                           \
  \
  BEGIN_OUT_OF_PLACE_SAVE(arc, tname, tval) \
    ASSERT_MSG(false, "trying to serialize an unserializable object"); \
  END_OUT_OF_PLACE_SAVE()                                           \


#endif

