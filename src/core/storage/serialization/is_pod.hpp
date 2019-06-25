/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_IS_POD_HPP
#define TURI_IS_POD_HPP
#include <type_traits>

namespace turi {

  /** \ingroup group_serialization
    \brief Inheriting from this type will force the serializer
    to treat the derived type as a POD type.
    */
  struct IS_POD_TYPE { };

  /**
   * \ingroup group_serialization
   *
   * \brief Tests if T is a POD type
   *
   * gl_is_pod<T>::value is true if T is a POD type (as determined by
   * boost::is_pod) or if T inherits from IS_POD_TYPE. gl_is_pod<T>::value
   * is false otherwise.
   */
  template <typename T>
  struct gl_is_pod{
    // it is a pod and is not an integer since we have special handlings for integers
    static constexpr bool value =  std::is_scalar<T>::value ||
        std::is_base_of<IS_POD_TYPE, T>::value;
    /*
     * BOOST_STATIC_CONSTANT(bool, value = (boost::type_traits::ice_or<
     *                                         boost::is_scalar<T>::value,
     *                                         boost::is_base_of<IS_POD_TYPE, T>::value
     *                                       >::value));
     */

    // standard POD detection is no good because things which contain pointers
    // are POD, but are not serializable
    // (T is POD and  T is not an integer of size >= 2)
    /*BOOST_STATIC_CONSTANT(bool, value =
                          (
                           boost::type_traits::ice_and<
                             boost::is_pod<T>::value,
                             boost::type_traits::ice_not<
                               boost::type_traits::ice_and<
                                 boost::is_integral<T>::value,
                                 sizeof(T) >= 2
                                 >::value
                               >::value
                             >::value
                          ));*/

  };

  /// \internal

  template <typename T>
  struct gl_is_pod_or_scaler{
    static constexpr bool value =  std::is_scalar<T>::value || gl_is_pod<T>::value;
  };
}

#endif
