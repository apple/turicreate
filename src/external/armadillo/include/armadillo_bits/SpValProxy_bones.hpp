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


/**
 * Sparse value proxy class, meant to prevent 0s from being added to sparse
 * matrices.  T1 should be either SpMat or SpSubview, and if it's not, bad news
 * is probably coming.  This class only uses T1::add_element() and
 * T1::delete_element().
 */
template<typename T1>
class SpValProxy
  {
  public:

  typedef typename T1::elem_type eT; // Convenience typedef

  friend class SpMat<eT>;
  friend class SpSubview<eT>;

  /**
   * Create the sparse value proxy.
   * Otherwise, pass a pointer to a reference of the value.
   */
  arma_inline SpValProxy(uword row, uword col, T1& in_parent, eT* in_val_ptr = NULL);

  //! For swapping operations.
  arma_inline SpValProxy& operator=(const SpValProxy& rhs);
  template<typename T2>
  arma_inline SpValProxy& operator=(const SpValProxy<T2>& rhs);

  //! Overload all of the potential operators.

  //! First, the ones that could modify a value.
  arma_inline SpValProxy& operator=(const eT rhs);
  arma_inline SpValProxy& operator+=(const eT rhs);
  arma_inline SpValProxy& operator-=(const eT rhs);
  arma_inline SpValProxy& operator*=(const eT rhs);
  arma_inline SpValProxy& operator/=(const eT rhs);

  arma_inline SpValProxy& operator++();
  arma_inline SpValProxy& operator--();
  arma_inline eT operator++(const int);
  arma_inline eT operator--(const int);

  //! This will work for any other operations that do not modify a value.
  arma_inline operator eT() const;


  private:

  // Deletes the element if it is zero.  Does not check if val_ptr == NULL!
  arma_inline arma_hot void check_zero();

  arma_aligned const uword row;
  arma_aligned const uword col;

  arma_aligned eT* val_ptr;

  arma_aligned T1& parent; // We will call this object if we need to insert or delete an element.
  };



//! @}
