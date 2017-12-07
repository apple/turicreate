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


//! \addtogroup SpBase
//! @{



template<typename elem_type, typename derived>
arma_inline
const derived&
SpBase<elem_type,derived>::get_ref() const
  {
  return static_cast<const derived&>(*this);
  }



template<typename elem_type, typename derived>
inline
const SpOp<derived, spop_htrans>
SpBase<elem_type,derived>::t() const
  {
  return SpOp<derived,spop_htrans>( (*this).get_ref() );
  }


template<typename elem_type, typename derived>
inline
const SpOp<derived, spop_htrans>
SpBase<elem_type,derived>::ht() const
  {
  return SpOp<derived, spop_htrans>( (*this).get_ref() );
  }



template<typename elem_type, typename derived>
inline
const SpOp<derived, spop_strans>
SpBase<elem_type,derived>::st() const
  {
  return SpOp<derived, spop_strans>( (*this).get_ref() );
  }



template<typename elem_type, typename derived>
inline
void
SpBase<elem_type,derived>::print(const std::string extra_text) const
  {
  const unwrap_spmat<derived> tmp( (*this).get_ref() );

  tmp.M.impl_print(extra_text);
  }



template<typename elem_type, typename derived>
inline
void
SpBase<elem_type,derived>::print(std::ostream& user_stream, const std::string extra_text) const
  {
  const unwrap_spmat<derived> tmp( (*this).get_ref() );

  tmp.M.impl_print(user_stream, extra_text);
  }



template<typename elem_type, typename derived>
inline
void
SpBase<elem_type,derived>::raw_print(const std::string extra_text) const
  {
  const unwrap_spmat<derived> tmp( (*this).get_ref() );

  tmp.M.impl_raw_print(extra_text);
  }



template<typename elem_type, typename derived>
inline
void
SpBase<elem_type,derived>::raw_print(std::ostream& user_stream, const std::string extra_text) const
  {
  const unwrap_spmat<derived> tmp( (*this).get_ref() );

  tmp.M.impl_raw_print(user_stream, extra_text);
  }



template<typename elem_type, typename derived>
inline
void
SpBase<elem_type, derived>::print_dense(const std::string extra_text) const
  {
  const unwrap_spmat<derived> tmp( (*this).get_ref() );

  tmp.M.impl_print_dense(extra_text);
  }



template<typename elem_type, typename derived>
inline
void
SpBase<elem_type, derived>::print_dense(std::ostream& user_stream, const std::string extra_text) const
  {
  const unwrap_spmat<derived> tmp( (*this).get_ref() );

  tmp.M.impl_print_dense(user_stream, extra_text);
  }



template<typename elem_type, typename derived>
inline
void
SpBase<elem_type, derived>::raw_print_dense(const std::string extra_text) const
  {
  const unwrap_spmat<derived> tmp( (*this).get_ref() );

  tmp.M.impl_raw_print_dense(extra_text);
  }



template<typename elem_type, typename derived>
inline
void
SpBase<elem_type, derived>::raw_print_dense(std::ostream& user_stream, const std::string extra_text) const
  {
  const unwrap_spmat<derived> tmp( (*this).get_ref() );

  tmp.M.impl_raw_print_dense(user_stream, extra_text);
  }



//
// extra functions defined in SpBase_eval_SpMat

template<typename elem_type, typename derived>
inline
const derived&
SpBase_eval_SpMat<elem_type, derived>::eval() const
  {
  arma_extra_debug_sigprint();

  return static_cast<const derived&>(*this);
  }



//
// extra functions defined in SpBase_eval_expr

template<typename elem_type, typename derived>
inline
SpMat<elem_type>
SpBase_eval_expr<elem_type, derived>::eval() const
  {
  arma_extra_debug_sigprint();

  return SpMat<elem_type>( static_cast<const derived&>(*this) );
  }



template<typename elem_type, typename derived>
inline
arma_warn_unused
elem_type
SpBase<elem_type, derived>::min() const
  {
  return spop_min::min( (*this).get_ref() );
  }



template<typename elem_type, typename derived>
inline
arma_warn_unused
elem_type
SpBase<elem_type, derived>::max() const
  {
  return spop_max::max( (*this).get_ref() );
  }



template<typename elem_type, typename derived>
inline
elem_type
SpBase<elem_type, derived>::min(uword& index_of_min_val) const
  {
  const SpProxy<derived> P( (*this).get_ref() );

  return spop_min::min_with_index(P, index_of_min_val);
  }



template<typename elem_type, typename derived>
inline
elem_type
SpBase<elem_type, derived>::max(uword& index_of_max_val) const
  {
  const SpProxy<derived> P( (*this).get_ref() );

  return spop_max::max_with_index(P, index_of_max_val);
  }



template<typename elem_type, typename derived>
inline
elem_type
SpBase<elem_type, derived>::min(uword& row_of_min_val, uword& col_of_min_val) const
  {
  const SpProxy<derived> P( (*this).get_ref() );

  uword index = 0;

  const elem_type val = spop_min::min_with_index(P, index);

  const uword local_n_rows = P.get_n_rows();

  row_of_min_val = index % local_n_rows;
  col_of_min_val = index / local_n_rows;

  return val;
  }



template<typename elem_type, typename derived>
inline
elem_type
SpBase<elem_type, derived>::max(uword& row_of_max_val, uword& col_of_max_val) const
  {
  const SpProxy<derived> P( (*this).get_ref() );

  uword index = 0;

  const elem_type val = spop_max::max_with_index(P, index);

  const uword local_n_rows = P.get_n_rows();

  row_of_max_val = index % local_n_rows;
  col_of_max_val = index / local_n_rows;

  return val;
  }



template<typename elem_type, typename derived>
inline
arma_warn_unused
uword
SpBase<elem_type,derived>::index_min() const
  {
  const SpProxy<derived> P( (*this).get_ref() );

  uword index = 0;

  if(P.get_n_elem() == 0)
    {
    arma_debug_check(true, "index_min(): object has no elements");
    }
  else
    {
    spop_min::min_with_index(P, index);
    }

  return index;
  }



template<typename elem_type, typename derived>
inline
arma_warn_unused
uword
SpBase<elem_type,derived>::index_max() const
  {
  const SpProxy<derived> P( (*this).get_ref() );

  uword index = 0;

  if(P.get_n_elem() == 0)
    {
    arma_debug_check(true, "index_max(): object has no elements");
    }
  else
    {
    spop_max::max_with_index(P, index);
    }

  return index;
  }



//! @}
