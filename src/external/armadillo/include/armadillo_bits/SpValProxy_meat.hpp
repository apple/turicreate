// Copyright 2008-2016 Conrad Sanderson (http://conradsanderson.id.au)
// Copyright 2008-2016 National ICT Australia (NICTA)
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
// ------------------------------------------------------------------------


//! \addtogroup SpValProxy
//! @{


//! SpValProxy implementation.
template<typename T1>
arma_inline
SpValProxy<T1>::SpValProxy(uword in_row, uword in_col, T1& in_parent, eT* in_val_ptr)
  : row(in_row)
  , col(in_col)
  , val_ptr(in_val_ptr)
  , parent(in_parent)
  {
  // Nothing to do.
  }



template<typename T1>
arma_inline
SpValProxy<T1>&
SpValProxy<T1>::operator=(const SpValProxy<T1>& rhs)
  {
  return (*this).operator=(eT(rhs));
  }



template<typename T1>
template<typename T2>
arma_inline
SpValProxy<T1>&
SpValProxy<T1>::operator=(const SpValProxy<T2>& rhs)
  {
  return (*this).operator=(eT(rhs));
  }



template<typename T1>
arma_inline
SpValProxy<T1>&
SpValProxy<T1>::operator=(const eT rhs)
  {
  if (rhs != eT(0)) // A nonzero element is being assigned.
    {

    if (val_ptr)
      {
      // The value exists and merely needs to be updated.
      *val_ptr = rhs;
      }

    else
      {
      // The value is nonzero and must be added.
      val_ptr = &parent.add_element(row, col, rhs);
      }

    }
  else // A zero is being assigned.~
    {

    if (val_ptr)
      {
      // The element exists, but we need to remove it, because it is being set to 0.
      parent.delete_element(row, col);
      val_ptr = NULL;
      }

    // If the element does not exist, we do not need to do anything at all.

    }

  return *this;
  }



template<typename T1>
arma_inline
SpValProxy<T1>&
SpValProxy<T1>::operator+=(const eT rhs)
  {
  if (val_ptr)
    {
    // The value already exists and merely needs to be updated.
    *val_ptr += rhs;
    check_zero();
    }
  else
    {
    if (rhs != eT(0))
      {
      // The value does not exist and must be added.
      val_ptr = &parent.add_element(row, col, rhs);
      }
    }

  return *this;
  }



template<typename T1>
arma_inline
SpValProxy<T1>&
SpValProxy<T1>::operator-=(const eT rhs)
  {
  if (val_ptr)
    {
    // The value already exists and merely needs to be updated.
    *val_ptr -= rhs;
    check_zero();
    }
  else
    {
    if (rhs != eT(0))
      {
      // The value does not exist and must be added.
      val_ptr = &parent.add_element(row, col, -rhs);
      }
    }

  return *this;
  }



template<typename T1>
arma_inline
SpValProxy<T1>&
SpValProxy<T1>::operator*=(const eT rhs)
  {
  if (rhs != eT(0))
    {

    if (val_ptr)
      {
      // The value already exists and merely needs to be updated.
      *val_ptr *= rhs;
      check_zero();
      }

    }
  else
    {

    if (val_ptr)
      {
      // Since we are multiplying by zero, the value can be deleted.
      parent.delete_element(row, col);
      val_ptr = NULL;
      }

    }

  return *this;
  }



template<typename T1>
arma_inline
SpValProxy<T1>&
SpValProxy<T1>::operator/=(const eT rhs)
  {
  if (rhs != eT(0)) // I hope this is true!
    {

    if (val_ptr)
      {
      *val_ptr /= rhs;
      check_zero();
      }

    }
  else
    {

    if (val_ptr)
      {
      *val_ptr /= rhs; // That is where it gets ugly.
      // Now check if it's 0.
      if (*val_ptr == eT(0))
        {
        parent.delete_element(row, col);
        val_ptr = NULL;
        }
      }

    else
      {
      eT val = eT(0) / rhs; // This may vary depending on type and implementation.

      if (val != eT(0))
        {
        // Ok, now we have to add it.
        val_ptr = &parent.add_element(row, col, val);
        }

      }
    }

  return *this;
  }



template<typename T1>
arma_inline
SpValProxy<T1>&
SpValProxy<T1>::operator++()
  {
  if (val_ptr)
    {
    (*val_ptr) += eT(1);
    check_zero();
    }

  else
    {
    val_ptr = &parent.add_element(row, col, eT(1));
    }

  return *this;
  }



template<typename T1>
arma_inline
SpValProxy<T1>&
SpValProxy<T1>::operator--()
  {
  if (val_ptr)
    {
    (*val_ptr) -= eT(1);
    check_zero();
    }

  else
    {
    val_ptr = &parent.add_element(row, col, eT(-1));
    }

  return *this;
  }



template<typename T1>
arma_inline
typename T1::elem_type
SpValProxy<T1>::operator++(const int)
  {
  if (val_ptr)
    {
    (*val_ptr) += eT(1);
    check_zero();
    }

  else
    {
    val_ptr = &parent.add_element(row, col, eT(1));
    }

  if (val_ptr) // It may have changed to now be 0.
    {
    return *(val_ptr) - eT(1);
    }
  else
    {
    return eT(0);
    }
  }



template<typename T1>
arma_inline
typename T1::elem_type
SpValProxy<T1>::operator--(const int)
  {
  if (val_ptr)
    {
    (*val_ptr) -= eT(1);
    check_zero();
    }

  else
    {
    val_ptr = &parent.add_element(row, col, eT(-1));
    }

  if (val_ptr) // It may have changed to now be 0.
    {
    return *(val_ptr) + eT(1);
    }
  else
    {
    return eT(0);
    }
  }



template<typename T1>
arma_inline
SpValProxy<T1>::operator eT() const
  {
  if (val_ptr)
    {
    return *val_ptr;
    }
  else
    {
    return eT(0);
    }
  }



template<typename T1>
arma_inline
arma_hot
void
SpValProxy<T1>::check_zero()
  {
  if (*val_ptr == eT(0))
    {
    parent.delete_element(row, col);
    val_ptr = NULL;
    }
  }



//! @}
