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


//! \addtogroup BaseCube
//! @{



template<typename elem_type, typename derived>
struct BaseCube_eval_Cube
  {
  arma_inline const derived& eval() const;
  };


template<typename elem_type, typename derived>
struct BaseCube_eval_expr
  {
  arma_inline Cube<elem_type> eval() const;   //!< force the immediate evaluation of a delayed expression
  };


template<typename elem_type, typename derived, bool condition>
struct BaseCube_eval {};

template<typename elem_type, typename derived>
struct BaseCube_eval<elem_type, derived, true>  { typedef BaseCube_eval_Cube<elem_type, derived>  result; };

template<typename elem_type, typename derived>
struct BaseCube_eval<elem_type, derived, false> { typedef BaseCube_eval_expr<elem_type, derived> result; };



//! Analog of the Base class, intended for cubes
template<typename elem_type, typename derived>
struct BaseCube
  : public BaseCube_eval<elem_type, derived, is_Cube<derived>::value>::result
  {
  arma_inline const derived& get_ref() const;

  inline void print(                           const std::string extra_text = "") const;
  inline void print(std::ostream& user_stream, const std::string extra_text = "") const;

  inline void raw_print(                           const std::string extra_text = "") const;
  inline void raw_print(std::ostream& user_stream, const std::string extra_text = "") const;

  inline arma_warn_unused elem_type min() const;
  inline arma_warn_unused elem_type max() const;

  inline arma_warn_unused uword index_min() const;
  inline arma_warn_unused uword index_max() const;
  };



//! @}
