/* Copyright © 2017 Apple Inc. All rights reserved.
 *
 * Use of this source code is governed by a BSD-3-clause license that can
 * be found in the LICENSE.txt file or at https://opensource.org/licenses/BSD-3-Clause
 */
#ifndef TURI_ATOMIC_OPS_HPP
#define TURI_ATOMIC_OPS_HPP

#include <stdint.h>


namespace turi {
  /**
   * \ingroup threading
     atomic instruction that is equivalent to the following:
     \code
     if (a==oldval) {
       a = newval;
       return true;
     }
     else {
       return false;
    }
    \endcode
  */
  template<typename T>
  bool atomic_compare_and_swap(T& a, T oldval, T newval) {
    return __sync_bool_compare_and_swap(&a, oldval, newval);
  };

  /**
   * \ingroup threading
     atomic instruction that is equivalent to the following:
     \code
     if (a==oldval) {
       a = newval;
       return true;
     }
     else {
       return false;
    }
    \endcode
  */
  template<typename T>
  bool atomic_compare_and_swap(volatile T& a,
                               T oldval,
                               T newval) {
    return __sync_bool_compare_and_swap(&a, oldval, newval);
  };

  /**
   * \ingroup threading
     atomic instruction that is equivalent to the following:
     \code
     if (a==oldval) {
       a = newval;
       return oldval;
     }
     else {
     return a;
    }
    \endcode
  */
  template<typename T>
  T atomic_compare_and_swap_val(T& a, T oldval, T newval) {
    return __sync_val_compare_and_swap(&a, oldval, newval);
  };

  /**
   * \ingroup threading
     atomic instruction that is equivalent to the following:
     \code
     if (a==oldval) {
       a = newval;
       return oldval;
     }
     else {
     return a;
    }
    \endcode
  */
  template<typename T>
  T atomic_compare_and_swap_val(volatile T& a,
                                T oldval,
                                T newval) {
    return __sync_val_compare_and_swap(&a, oldval, newval);
  };

  /**
   * \ingroup threading
     atomic instruction that is equivalent to the following:
     \code
     if (a==oldval) {
       a = newval;
       return true;
     }
     else {
       return false;
    }
    \endcode
  */
  template <>
  inline bool atomic_compare_and_swap(volatile double& a,
                                      double oldval,
                                      double newval) {
    volatile uint64_t* a_ptr = reinterpret_cast<volatile uint64_t*>(&a);
    const uint64_t* oldval_ptr = reinterpret_cast<const uint64_t*>(&oldval);
    const uint64_t* newval_ptr = reinterpret_cast<const uint64_t*>(&newval);
    return __sync_bool_compare_and_swap(a_ptr, *oldval_ptr, *newval_ptr);
  };

  /**
   * \ingroup threading
     atomic instruction that is equivalent to the following:
     \code
     if (a==oldval) {
       a = newval;
       return true;
     }
     else {
       return false;
    }
    \endcode
  */
  template <>
  inline bool atomic_compare_and_swap(volatile float& a,
                                      float oldval,
                                      float newval) {
    volatile uint32_t* a_ptr = reinterpret_cast<volatile uint32_t*>(&a);
    const uint32_t* oldval_ptr = reinterpret_cast<const uint32_t*>(&oldval);
    const uint32_t* newval_ptr = reinterpret_cast<const uint32_t*>(&newval);
    return __sync_bool_compare_and_swap(a_ptr, *oldval_ptr, *newval_ptr);
  };

  /**
    * \ingroup threading
    * \brief Atomically exchanges the values of a and b.
    * \warning This is not a full atomic exchange. Read of a,
    * and the write of b into a is atomic. But the write into b is not.
    */
  template<typename T>
  void atomic_exchange(T& a, T& b) {
    b = __sync_lock_test_and_set(&a, b);
  };

  /**
    * \ingroup threading
    * \brief Atomically exchanges the values of a and b.
    * \warning This is not a full atomic exchange. Read of a,
    * and the write of b into a is atomic. But the write into b is not.
    */
  template<typename T>
  void atomic_exchange(volatile T& a, T& b) {
    b = __sync_lock_test_and_set(&a, b);
  };

  /**
    * \ingroup threading
    * \brief Atomically sets a to the newval, returning the old value
    */
  template<typename T>
  T fetch_and_store(T& a, const T& newval) {
    return __sync_lock_test_and_set(&a, newval);
  };

   /** Atomically sets the max, returning the value of the atomic
    *  prior to setting the max value, or the existing value if
    *  nothing changed.  Atomic equivalent to:
    *
    *   old_max_value = max_value;
    *   max_value = std::max(max_value, new_value);
    *   return old_max_value;
    *
    */
  template<typename T>
  T atomic_set_max(T& max_value, T new_value) {
    T v = max_value;
    T oldval = v;
    if(v < new_value) {
      do {
        oldval = atomic_compare_and_swap_val(max_value, v, new_value);

        if(oldval == v) {
          // Change successful
          break;
        } else {
          // Change not successful, reset.
          v = oldval;
        }
      } while(v < new_value);
    }

    return oldval;
  }

   /** Atomically sets the max, returning the value of the atomic
    *  prior to this operation, or the existing value if
    *  nothing changed.  Atomic equivalent to:
    *
    *   old_max_value = max_value;
    *   max_value = std::max(max_value, new_value);
    *   return old_max_value;
    *
    *  \overload
    */
  template<typename T>
  T atomic_set_max(volatile T& max_value, T new_value) {
    T v = max_value;
    T oldval = v;
    if(v < new_value) {
      do {
        oldval = atomic_compare_and_swap_val(max_value, v, new_value);

        if(oldval == v) {
          // Change successful
          break;
        } else {
          // Change not successful, reset.
          v = oldval;
        }
      } while(v < new_value);
    }
    return oldval;
  }

   /** Atomically sets the min, returning the value of the atomic
    *  prior to this operation.  Atomic equivalent to:
    *
    *    old_min_value = min_value;
    *    min_value = std::min(min_value, new_value);
    *    return old_min_value;
    *
    *  \overload
    */
  template<typename T>
  T atomic_set_min(T& min_value, T new_value) {
    T v = min_value;
    T oldval = v;
    if(v > new_value) {
      do {
        oldval = atomic_compare_and_swap_val(min_value, v, new_value);

        if(oldval == v) {
          // Change successful
          break;
        } else {
          // Change not successful, reset.
          v = oldval;
        }
      } while(v > new_value);
    }
    return oldval;
  }

   /** Atomically sets the min, returning the value of the atomic
    *  prior to this operation.  Atomic equivalent to:
    *
    *    old_min_value = min_value;
    *    min_value = std::min(min_value, new_value);
    *    return old_min_value;
    *
    *  \overload
    */
  template<typename T>
  T atomic_set_min(volatile T& min_value, T new_value) {
    T v = min_value;
    T oldval = v;
    if(v > new_value) {
      do {
        oldval = atomic_compare_and_swap_val(min_value, v, new_value);

        if(oldval == v) {
          // Change successful
          break;
        } else {
          // Change not successful, reset.
          v = oldval;
        }
      } while(v > new_value);
    }
    return oldval;
  }

  /**  Atomically increments the value, and returns the value of the
   *   atomic prior to this operation.   Equivalent to
   *
   *     old_value = value;
   *     value += increment;
   *     return value;
   */
  template<typename T>
  T atomic_increment(T& value, int increment = 1) {
    return __sync_fetch_and_add(&value, increment);
  }

  /**  Atomically increments the value, and returns the value of the
   *   atomic prior to this operation.   Equivalent to
   *
   *     old_value = value;
   *     value += increment;
   *     return value;
   *
   *   \overload
   */
  template<typename T>
  T atomic_increment(volatile T& value, int increment = 1) {
    return __sync_fetch_and_add(&value, increment);
  }


}
#endif
