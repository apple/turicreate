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



template<typename derived>
struct Base_inv_yes
  {
  arma_inline const Op<derived,op_inv> i() const;   //!< matrix inverse

  arma_deprecated inline const Op<derived,op_inv> i(const bool ) const;   //!< kept only for compatibility with old user code
  arma_deprecated inline const Op<derived,op_inv> i(const char*) const;   //!< kept only for compatibility with old user code
  };


template<typename derived>
struct Base_inv_no
  {
  };


template<typename derived, bool condition>
struct Base_inv {};

template<typename derived>
struct Base_inv<derived, true>  { typedef Base_inv_yes<derived> result; };

template<typename derived>
struct Base_inv<derived, false> { typedef Base_inv_no<derived>  result; };



template<typename elem_type, typename derived>
struct Base_eval_Mat
  {
  arma_inline const derived& eval() const;
  };


template<typename elem_type, typename derived>
struct Base_eval_expr
  {
  arma_inline Mat<elem_type> eval() const;   //!< force the immediate evaluation of a delayed expression
  };


template<typename elem_type, typename derived, bool condition>
struct Base_eval {};

template<typename elem_type, typename derived>
struct Base_eval<elem_type, derived, true>  { typedef Base_eval_Mat<elem_type, derived>  result; };

template<typename elem_type, typename derived>
struct Base_eval<elem_type, derived, false> { typedef Base_eval_expr<elem_type, derived> result; };



template<typename derived>
struct Base_trans_cx
  {
  arma_inline const Op<derived,op_htrans>  t() const;
  arma_inline const Op<derived,op_htrans> ht() const;
  arma_inline const Op<derived,op_strans> st() const;  // simple transpose: no complex conjugates
  };


template<typename derived>
struct Base_trans_default
  {
  arma_inline const Op<derived,op_htrans>  t() const;
  arma_inline const Op<derived,op_htrans> ht() const;
  arma_inline const Op<derived,op_htrans> st() const;  // return op_htrans instead of op_strans, as it's handled better by matrix multiplication code
  };


template<typename derived, bool condition>
struct Base_trans {};

template<typename derived>
struct Base_trans<derived, true>  { typedef Base_trans_cx<derived>      result; };

template<typename derived>
struct Base_trans<derived, false> { typedef Base_trans_default<derived> result; };



//! Class for static polymorphism, modelled after the "Curiously Recurring Template Pattern" (CRTP).
//! Used for type-safe downcasting in functions that restrict their input(s) to be classes that are
//! derived from Base (e.g. Mat, Op, Glue, diagview, subview).
//! A Base object can be converted to a Mat object by the unwrap class.

template<typename elem_type, typename derived>
struct Base
  : public Base_inv<derived, is_supported_blas_type<elem_type>::value>::result
  , public Base_eval<elem_type, derived, is_Mat<derived>::value>::result
  , public Base_trans<derived, is_cx<elem_type>::value>::result
  {
  arma_inline const derived& get_ref() const;

  inline void print(                           const std::string extra_text = "") const;
  inline void print(std::ostream& user_stream, const std::string extra_text = "") const;

  inline void raw_print(                           const std::string extra_text = "") const;
  inline void raw_print(std::ostream& user_stream, const std::string extra_text = "") const;

  inline arma_warn_unused elem_type min() const;
  inline arma_warn_unused elem_type max() const;

  inline elem_type min(uword& index_of_min_val) const;
  inline elem_type max(uword& index_of_max_val) const;

  inline elem_type min(uword& row_of_min_val, uword& col_of_min_val) const;
  inline elem_type max(uword& row_of_max_val, uword& col_of_max_val) const;

  inline arma_warn_unused uword index_min() const;
  inline arma_warn_unused uword index_max() const;
  };



//! @}
