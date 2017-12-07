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


//! \addtogroup Base
//! @{



template<typename elem_type, typename derived>
arma_inline
const derived&
Base<elem_type,derived>::get_ref() const
  {
  return static_cast<const derived&>(*this);
  }



template<typename elem_type, typename derived>
inline
void
Base<elem_type,derived>::print(const std::string extra_text) const
  {
  if(is_op_strans<derived>::value || is_op_htrans<derived>::value)
    {
    const Proxy<derived> P( (*this).get_ref() );

    const quasi_unwrap< typename Proxy<derived>::stored_type > tmp(P.Q);

    tmp.M.impl_print(extra_text);
    }
  else
    {
    const quasi_unwrap<derived> tmp( (*this).get_ref() );

    tmp.M.impl_print(extra_text);
    }
  }



template<typename elem_type, typename derived>
inline
void
Base<elem_type,derived>::print(std::ostream& user_stream, const std::string extra_text) const
  {
  if(is_op_strans<derived>::value || is_op_htrans<derived>::value)
    {
    const Proxy<derived> P( (*this).get_ref() );

    const quasi_unwrap< typename Proxy<derived>::stored_type > tmp(P.Q);

    tmp.M.impl_print(user_stream, extra_text);
    }
  else
    {
    const quasi_unwrap<derived> tmp( (*this).get_ref() );

    tmp.M.impl_print(user_stream, extra_text);
    }
  }



template<typename elem_type, typename derived>
inline
void
Base<elem_type,derived>::raw_print(const std::string extra_text) const
  {
  if(is_op_strans<derived>::value || is_op_htrans<derived>::value)
    {
    const Proxy<derived> P( (*this).get_ref() );

    const quasi_unwrap< typename Proxy<derived>::stored_type > tmp(P.Q);

    tmp.M.impl_raw_print(extra_text);
    }
  else
    {
    const quasi_unwrap<derived> tmp( (*this).get_ref() );

    tmp.M.impl_raw_print(extra_text);
    }
  }



template<typename elem_type, typename derived>
inline
void
Base<elem_type,derived>::raw_print(std::ostream& user_stream, const std::string extra_text) const
  {
  if(is_op_strans<derived>::value || is_op_htrans<derived>::value)
    {
    const Proxy<derived> P( (*this).get_ref() );

    const quasi_unwrap< typename Proxy<derived>::stored_type > tmp(P.Q);

    tmp.M.impl_raw_print(user_stream, extra_text);
    }
  else
    {
    const quasi_unwrap<derived> tmp( (*this).get_ref() );

    tmp.M.impl_raw_print(user_stream, extra_text);
    }
  }



template<typename elem_type, typename derived>
inline
arma_warn_unused
elem_type
Base<elem_type,derived>::min() const
  {
  return op_min::min( (*this).get_ref() );
  }



template<typename elem_type, typename derived>
inline
arma_warn_unused
elem_type
Base<elem_type,derived>::max() const
  {
  return op_max::max( (*this).get_ref() );
  }



template<typename elem_type, typename derived>
inline
elem_type
Base<elem_type,derived>::min(uword& index_of_min_val) const
  {
  const Proxy<derived> P( (*this).get_ref() );

  return op_min::min_with_index(P, index_of_min_val);
  }



template<typename elem_type, typename derived>
inline
elem_type
Base<elem_type,derived>::max(uword& index_of_max_val) const
  {
  const Proxy<derived> P( (*this).get_ref() );

  return op_max::max_with_index(P, index_of_max_val);
  }



template<typename elem_type, typename derived>
inline
elem_type
Base<elem_type,derived>::min(uword& row_of_min_val, uword& col_of_min_val) const
  {
  const Proxy<derived> P( (*this).get_ref() );

  uword index = 0;

  const elem_type val = op_min::min_with_index(P, index);

  const uword local_n_rows = P.get_n_rows();

  row_of_min_val = index % local_n_rows;
  col_of_min_val = index / local_n_rows;

  return val;
  }



template<typename elem_type, typename derived>
inline
elem_type
Base<elem_type,derived>::max(uword& row_of_max_val, uword& col_of_max_val) const
  {
  const Proxy<derived> P( (*this).get_ref() );

  uword index = 0;

  const elem_type val = op_max::max_with_index(P, index);

  const uword local_n_rows = P.get_n_rows();

  row_of_max_val = index % local_n_rows;
  col_of_max_val = index / local_n_rows;

  return val;
  }



template<typename elem_type, typename derived>
inline
arma_warn_unused
uword
Base<elem_type,derived>::index_min() const
  {
  const Proxy<derived> P( (*this).get_ref() );

  uword index = 0;

  if(P.get_n_elem() == 0)
    {
    arma_debug_check(true, "index_min(): object has no elements");
    }
  else
    {
    op_min::min_with_index(P, index);
    }

  return index;
  }



template<typename elem_type, typename derived>
inline
arma_warn_unused
uword
Base<elem_type,derived>::index_max() const
  {
  const Proxy<derived> P( (*this).get_ref() );

  uword index = 0;

  if(P.get_n_elem() == 0)
    {
    arma_debug_check(true, "index_max(): object has no elements");
    }
  else
    {
    op_max::max_with_index(P, index);
    }

  return index;
  }



//
// extra functions defined in Base_inv_yes

template<typename derived>
arma_inline
const Op<derived,op_inv>
Base_inv_yes<derived>::i() const
  {
  return Op<derived,op_inv>(static_cast<const derived&>(*this));
  }



template<typename derived>
arma_deprecated
inline
const Op<derived,op_inv>
Base_inv_yes<derived>::i(const bool) const   // argument kept only for compatibility with old user code
  {
  // arma_debug_warn(".i(bool) is deprecated and will be removed; change to .i()");

  return Op<derived,op_inv>(static_cast<const derived&>(*this));
  }



template<typename derived>
arma_deprecated
inline
const Op<derived,op_inv>
Base_inv_yes<derived>::i(const char*) const   // argument kept only for compatibility with old user code
  {
  // arma_debug_warn(".i(char*) is deprecated and will be removed; change to .i()");

  return Op<derived,op_inv>(static_cast<const derived&>(*this));
  }



//
// extra functions defined in Base_eval_Mat

template<typename elem_type, typename derived>
arma_inline
const derived&
Base_eval_Mat<elem_type, derived>::eval() const
  {
  arma_extra_debug_sigprint();

  return static_cast<const derived&>(*this);
  }



//
// extra functions defined in Base_eval_expr

template<typename elem_type, typename derived>
arma_inline
Mat<elem_type>
Base_eval_expr<elem_type, derived>::eval() const
  {
  arma_extra_debug_sigprint();

  return Mat<elem_type>( static_cast<const derived&>(*this) );
  }



//
// extra functions defined in Base_trans_cx

template<typename derived>
arma_inline
const Op<derived,op_htrans>
Base_trans_cx<derived>::t() const
  {
  return Op<derived,op_htrans>( static_cast<const derived&>(*this) );
  }



template<typename derived>
arma_inline
const Op<derived,op_htrans>
Base_trans_cx<derived>::ht() const
  {
  return Op<derived,op_htrans>( static_cast<const derived&>(*this) );
  }



template<typename derived>
arma_inline
const Op<derived,op_strans>
Base_trans_cx<derived>::st() const
  {
  return Op<derived,op_strans>( static_cast<const derived&>(*this) );
  }



//
// extra functions defined in Base_trans_default

template<typename derived>
arma_inline
const Op<derived,op_htrans>
Base_trans_default<derived>::t() const
  {
  return Op<derived,op_htrans>( static_cast<const derived&>(*this) );
  }



template<typename derived>
arma_inline
const Op<derived,op_htrans>
Base_trans_default<derived>::ht() const
  {
  return Op<derived,op_htrans>( static_cast<const derived&>(*this) );
  }



template<typename derived>
arma_inline
const Op<derived,op_htrans>
Base_trans_default<derived>::st() const
  {
  return Op<derived,op_htrans>( static_cast<const derived&>(*this) );
  }



//! @}
