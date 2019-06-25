/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_HAS_LOAD_HPP
#define TURI_HAS_LOAD_HPP

#include <typeinfo>
#include <type_traits>

namespace turi {
  namespace archive_detail {

    /** SFINAE method to detect if a class T
     * implements a function void T::load(ArcType&)
     *
     * If T implements the method, has_load_method<ArcType,T>::value will be
     * true. Otherwise it will be false
     */
    template<typename ArcType, typename T>
    struct has_load_method
    {
      template<typename U, void (U::*)(ArcType&)> struct SFINAE {};
      template<typename U> static char Test(SFINAE<U, &U::load>*);
      template<typename U> static int Test(...);
      static const bool value = sizeof(Test<T>(0)) == sizeof(char);
    };

    /**
     *  load_or_fail<ArcType, T>(arc, t)
     *  will call this version of the function if
     *  T implements void T::load(ArcType&).
     *
     * load_or_fail<ArcType, T>(arc, t) will therefore load the class successfully
     * if T implements the load function correctly. Otherwise, calling
     * load_or_fail will print an error message.
     */
    template <typename ArcType, typename ValueType>
    typename std::enable_if<has_load_method<ArcType, ValueType>::value, void>::type
    load_or_fail(ArcType& o, ValueType &t) {
      t.load(o);
    }

     /**
     *  load_or_fail<ArcType, T>(arc, t)
     *  will call this version of the function if
     *
     * load_or_fail<ArcType, T>(arc, t) will therefore load the class successfully
     * if T implements the load function correctly. Otherwise, calling
     * load_or_fail will print an error message.
     * T does not implement void T::load(ArcType&).
     */
    template <typename ArcType, typename ValueType>
    typename std::enable_if<!has_load_method<ArcType, ValueType>::value, void>::type
    load_or_fail(ArcType& o, ValueType &t) {
      ASSERT_MSG(false, "Trying to deserializable type %s without valid load method.", typeid(ValueType).name());
    }

  }  // archive_detail
}  // turicreate

#endif
