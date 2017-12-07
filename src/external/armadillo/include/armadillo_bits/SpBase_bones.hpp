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
struct SpBase_eval_SpMat
  {
  inline const derived& eval() const;
  };


template<typename elem_type, typename derived>
struct SpBase_eval_expr
  {
  inline SpMat<elem_type> eval() const;   //!< force the immediate evaluation of a delayed expression
  };


template<typename elem_type, typename derived, bool condition>
struct SpBase_eval {};

template<typename elem_type, typename derived>
struct SpBase_eval<elem_type, derived, true>  { typedef SpBase_eval_SpMat<elem_type, derived> result; };

template<typename elem_type, typename derived>
struct SpBase_eval<elem_type, derived, false> { typedef SpBase_eval_expr<elem_type, derived>  result; };



template<typename elem_type, typename derived>
struct SpBase
  : public SpBase_eval<elem_type, derived, is_SpMat<derived>::value>::result
  {
  arma_inline const derived& get_ref() const;

  inline const SpOp<derived,spop_htrans>  t() const;  //!< Hermitian transpose
  inline const SpOp<derived,spop_htrans> ht() const;  //!< Hermitian transpose
  inline const SpOp<derived,spop_strans> st() const;  //!< simple transpose

  inline void print(                           const std::string extra_text = "") const;
  inline void print(std::ostream& user_stream, const std::string extra_text = "") const;

  inline void raw_print(                           const std::string extra_text = "") const;
  inline void raw_print(std::ostream& user_stream, const std::string extra_text = "") const;

  inline void print_dense(                           const std::string extra_text = "") const;
  inline void print_dense(std::ostream& user_stream, const std::string extra_text = "") const;

  inline void raw_print_dense(                           const std::string extra_text = "") const;
  inline void raw_print_dense(std::ostream& user_stream, const std::string extra_text = "") const;

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
