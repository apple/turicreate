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


//! \addtogroup GenCube
//! @{



template<typename eT, typename gen_type>
arma_inline
GenCube<eT, gen_type>::GenCube(const uword in_n_rows, const uword in_n_cols, const uword in_n_slices)
  : n_rows  (in_n_rows  )
  , n_cols  (in_n_cols  )
  , n_slices(in_n_slices)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT, typename gen_type>
arma_inline
GenCube<eT, gen_type>::~GenCube()
  {
  arma_extra_debug_sigprint();
  }



template<typename eT, typename gen_type>
arma_inline
eT
GenCube<eT, gen_type>::operator[](const uword) const
  {
  return (*this).generate();
  }



template<typename eT, typename gen_type>
arma_inline
eT
GenCube<eT, gen_type>::at(const uword, const uword, const uword) const
  {
  return (*this).generate();
  }



template<typename eT, typename gen_type>
arma_inline
eT
GenCube<eT, gen_type>::at_alt(const uword) const
  {
  return (*this).generate();
  }



template<typename eT, typename gen_type>
inline
void
GenCube<eT, gen_type>::apply(Cube<eT>& out) const
  {
  arma_extra_debug_sigprint();

  // NOTE: we're assuming that the cube has already been set to the correct size;
  // this is done by either the Cube contructor or operator=()

       if(is_same_type<gen_type, gen_ones >::yes) { out.ones();  }
  else if(is_same_type<gen_type, gen_zeros>::yes) { out.zeros(); }
  else if(is_same_type<gen_type, gen_randu>::yes) { out.randu(); }
  else if(is_same_type<gen_type, gen_randn>::yes) { out.randn(); }
  }



template<typename eT, typename gen_type>
inline
void
GenCube<eT, gen_type>::apply_inplace_plus(Cube<eT>& out) const
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, out.n_slices, n_rows, n_cols, n_slices, "addition");


        eT*   out_mem = out.memptr();
  const uword n_elem  = out.n_elem;

  uword i,j;

  for(i=0, j=1; j<n_elem; i+=2, j+=2)
    {
    const eT tmp_i = (*this).generate();
    const eT tmp_j = (*this).generate();

    out_mem[i] += tmp_i;
    out_mem[j] += tmp_j;
    }

  if(i < n_elem)
    {
    out_mem[i] += (*this).generate();
    }
  }




template<typename eT, typename gen_type>
inline
void
GenCube<eT, gen_type>::apply_inplace_minus(Cube<eT>& out) const
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, out.n_slices, n_rows, n_cols, n_slices, "subtraction");


        eT*   out_mem = out.memptr();
  const uword n_elem  = out.n_elem;

  uword i,j;

  for(i=0, j=1; j<n_elem; i+=2, j+=2)
    {
    const eT tmp_i = (*this).generate();
    const eT tmp_j = (*this).generate();

    out_mem[i] -= tmp_i;
    out_mem[j] -= tmp_j;
    }

  if(i < n_elem)
    {
    out_mem[i] -= (*this).generate();
    }
  }




template<typename eT, typename gen_type>
inline
void
GenCube<eT, gen_type>::apply_inplace_schur(Cube<eT>& out) const
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, out.n_slices, n_rows, n_cols, n_slices, "element-wise multiplication");


        eT*   out_mem = out.memptr();
  const uword n_elem  = out.n_elem;

  uword i,j;

  for(i=0, j=1; j<n_elem; i+=2, j+=2)
    {
    const eT tmp_i = (*this).generate();
    const eT tmp_j = (*this).generate();

    out_mem[i] *= tmp_i;
    out_mem[j] *= tmp_j;
    }

  if(i < n_elem)
    {
    out_mem[i] *= (*this).generate();
    }
  }




template<typename eT, typename gen_type>
inline
void
GenCube<eT, gen_type>::apply_inplace_div(Cube<eT>& out) const
  {
  arma_extra_debug_sigprint();

  arma_debug_assert_same_size(out.n_rows, out.n_cols, out.n_slices, n_rows, n_cols, n_slices, "element-wise division");


        eT*   out_mem = out.memptr();
  const uword n_elem  = out.n_elem;

  uword i,j;

  for(i=0, j=1; j<n_elem; i+=2, j+=2)
    {
    const eT tmp_i = (*this).generate();
    const eT tmp_j = (*this).generate();

    out_mem[i] /= tmp_i;
    out_mem[j] /= tmp_j;
    }

  if(i < n_elem)
    {
    out_mem[i] /= (*this).generate();
    }
  }



template<typename eT, typename gen_type>
inline
void
GenCube<eT, gen_type>::apply(subview_cube<eT>& out) const
  {
  arma_extra_debug_sigprint();

  // NOTE: we're assuming that the subcube has the same dimensions as the GenCube object
  // this is checked by subview_cube::operator=()

       if(is_same_type<gen_type, gen_ones >::yes) { out.ones();  }
  else if(is_same_type<gen_type, gen_zeros>::yes) { out.zeros(); }
  else if(is_same_type<gen_type, gen_randu>::yes) { out.randu(); }
  else if(is_same_type<gen_type, gen_randn>::yes) { out.randn(); }
  }



//! @}
