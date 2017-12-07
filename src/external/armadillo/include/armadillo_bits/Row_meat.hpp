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


//! \addtogroup Row
//! @{


//! construct an empty row vector
template<typename eT>
inline
Row<eT>::Row()
  : Mat<eT>(arma_vec_indicator(), 2)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
Row<eT>::Row(const Row<eT>& X)
  : Mat<eT>(arma_vec_indicator(), 1, X.n_elem, 2)
  {
  arma_extra_debug_sigprint();

  arrayops::copy((*this).memptr(), X.memptr(), X.n_elem);
  }



//! construct a row vector with the specified number of n_elem
template<typename eT>
inline
Row<eT>::Row(const uword in_n_elem)
  : Mat<eT>(arma_vec_indicator(), 1, in_n_elem, 2)
  {
  arma_extra_debug_sigprint();
  }



template<typename eT>
inline
Row<eT>::Row(const uword in_n_rows, const uword in_n_cols)
  : Mat<eT>(arma_vec_indicator(), 0, 0, 2)
  {
  arma_extra_debug_sigprint();

  Mat<eT>::init_warm(in_n_rows, in_n_cols);
  }



template<typename eT>
inline
Row<eT>::Row(const SizeMat& s)
  : Mat<eT>(arma_vec_indicator(), 0, 0, 2)
  {
  arma_extra_debug_sigprint();

  Mat<eT>::init_warm(s.n_rows, s.n_cols);
  }



template<typename eT>
template<typename fill_type>
inline
Row<eT>::Row(const uword in_n_elem, const fill::fill_class<fill_type>& f)
  : Mat<eT>(arma_vec_indicator(), 1, in_n_elem, 2)
  {
  arma_extra_debug_sigprint();

  (*this).fill(f);
  }



template<typename eT>
template<typename fill_type>
inline
Row<eT>::Row(const uword in_n_rows, const uword in_n_cols, const fill::fill_class<fill_type>& f)
  : Mat<eT>(arma_vec_indicator(), 0, 0, 2)
  {
  arma_extra_debug_sigprint();

  Mat<eT>::init_warm(in_n_rows, in_n_cols);

  (*this).fill(f);
  }



template<typename eT>
template<typename fill_type>
inline
Row<eT>::Row(const SizeMat& s, const fill::fill_class<fill_type>& f)
  : Mat<eT>(arma_vec_indicator(), 0, 0, 2)
  {
  arma_extra_debug_sigprint();

  Mat<eT>::init_warm(s.n_rows, s.n_cols);

  (*this).fill(f);
  }



template<typename eT>
inline
Row<eT>::Row(const char* text)
  : Mat<eT>(arma_vec_indicator(), 2)
  {
  arma_extra_debug_sigprint();

  (*this).operator=(text);
  }



template<typename eT>
inline
Row<eT>&
Row<eT>::operator=(const char* text)
  {
  arma_extra_debug_sigprint();

  Mat<eT> tmp(text);

  arma_debug_check( ((tmp.n_elem > 0) && (tmp.is_vec() == false)), "Mat::init(): requested size is not compatible with row vector layout" );

  access::rw(tmp.n_rows) = 1;
  access::rw(tmp.n_cols) = tmp.n_elem;

  (*this).steal_mem(tmp);

  return *this;
  }



template<typename eT>
inline
Row<eT>::Row(const std::string& text)
  : Mat<eT>(arma_vec_indicator(), 2)
  {
  arma_extra_debug_sigprint();

  (*this).operator=(text);
  }



template<typename eT>
inline
Row<eT>&
Row<eT>::operator=(const std::string& text)
  {
  arma_extra_debug_sigprint();

  Mat<eT> tmp(text);

  arma_debug_check( ((tmp.n_elem > 0) && (tmp.is_vec() == false)), "Mat::init(): requested size is not compatible with row vector layout" );

  access::rw(tmp.n_rows) = 1;
  access::rw(tmp.n_cols) = tmp.n_elem;

  (*this).steal_mem(tmp);

  return *this;
  }



//! create a row vector from std::vector
template<typename eT>
inline
Row<eT>::Row(const std::vector<eT>& x)
  : Mat<eT>(arma_vec_indicator(), 1, uword(x.size()), 2)
  {
  arma_extra_debug_sigprint_this(this);

  if(x.size() > 0)
    {
    arrayops::copy( Mat<eT>::memptr(), &(x[0]), uword(x.size()) );
    }
  }



//! create a row vector from std::vector
template<typename eT>
inline
Row<eT>&
Row<eT>::operator=(const std::vector<eT>& x)
  {
  arma_extra_debug_sigprint();

  Mat<eT>::init_warm(1, uword(x.size()));

  if(x.size() > 0)
    {
    arrayops::copy( Mat<eT>::memptr(), &(x[0]), uword(x.size()) );
    }

  return *this;
  }



#if defined(ARMA_USE_CXX11)

  template<typename eT>
  inline
  Row<eT>::Row(const std::initializer_list<eT>& list)
    : Mat<eT>(arma_vec_indicator(), 2)
    {
    arma_extra_debug_sigprint();

    (*this).operator=(list);
    }



  template<typename eT>
  inline
  Row<eT>&
  Row<eT>::operator=(const std::initializer_list<eT>& list)
    {
    arma_extra_debug_sigprint();

    Mat<eT> tmp(list);

    arma_debug_check( ((tmp.n_elem > 0) && (tmp.is_vec() == false)), "Mat::init(): requested size is not compatible with row vector layout" );

    access::rw(tmp.n_rows) = 1;
    access::rw(tmp.n_cols) = tmp.n_elem;

    (*this).steal_mem(tmp);

    return *this;
    }



  template<typename eT>
  inline
  Row<eT>::Row(Row<eT>&& X)
    : Mat<eT>(arma_vec_indicator(), 2)
    {
    arma_extra_debug_sigprint(arma_str::format("this = %x   X = %x") % this % &X);

    access::rw(Mat<eT>::n_rows) = 1;
    access::rw(Mat<eT>::n_cols) = X.n_cols;
    access::rw(Mat<eT>::n_elem) = X.n_elem;

    if( ((X.mem_state == 0) && (X.n_elem > arma_config::mat_prealloc)) || (X.mem_state == 1) || (X.mem_state == 2) )
      {
      access::rw(Mat<eT>::mem_state) = X.mem_state;
      access::rw(Mat<eT>::mem)       = X.mem;

      access::rw(X.n_rows)    = 1;
      access::rw(X.n_cols)    = 0;
      access::rw(X.n_elem)    = 0;
      access::rw(X.mem_state) = 0;
      access::rw(X.mem)       = 0;
      }
    else
      {
      (*this).init_cold();

      arrayops::copy( (*this).memptr(), X.mem, X.n_elem );

      if( (X.mem_state == 0) && (X.n_elem <= arma_config::mat_prealloc) )
        {
        access::rw(X.n_rows) = 1;
        access::rw(X.n_cols) = 0;
        access::rw(X.n_elem) = 0;
        access::rw(X.mem)    = 0;
        }
      }
    }



  template<typename eT>
  inline
  Row<eT>&
  Row<eT>::operator=(Row<eT>&& X)
    {
    arma_extra_debug_sigprint(arma_str::format("this = %x   X = %x") % this % &X);

    (*this).steal_mem(X);

    if( (X.mem_state == 0) && (X.n_elem <= arma_config::mat_prealloc) && (this != &X) )
      {
      access::rw(X.n_rows) = 1;
      access::rw(X.n_cols) = 0;
      access::rw(X.n_elem) = 0;
      access::rw(X.mem)    = 0;
      }

    return *this;
    }

#endif



template<typename eT>
inline
Row<eT>&
Row<eT>::operator=(const eT val)
  {
  arma_extra_debug_sigprint();

  Mat<eT>::operator=(val);

  return *this;
  }



template<typename eT>
inline
Row<eT>&
Row<eT>::operator=(const Row<eT>& X)
  {
  arma_extra_debug_sigprint();

  Mat<eT>::operator=(X);

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Row<eT>::Row(const Base<eT,T1>& X)
  : Mat<eT>(arma_vec_indicator(), 2)
  {
  arma_extra_debug_sigprint();

  Mat<eT>::operator=(X.get_ref());
  }



template<typename eT>
template<typename T1>
inline
Row<eT>&
Row<eT>::operator=(const Base<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  Mat<eT>::operator=(X.get_ref());

  return *this;
  }



template<typename eT>
template<typename T1>
inline
Row<eT>::Row(const SpBase<eT,T1>& X)
  : Mat<eT>(arma_vec_indicator(), 2)
  {
  arma_extra_debug_sigprint();

  Mat<eT>::operator=(X.get_ref());
  }



template<typename eT>
template<typename T1>
inline
Row<eT>&
Row<eT>::operator=(const SpBase<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  Mat<eT>::operator=(X.get_ref());

  return *this;
  }



//! construct a row vector from a given auxiliary array
template<typename eT>
inline
Row<eT>::Row(eT* aux_mem, const uword aux_length, const bool copy_aux_mem, const bool strict)
  : Mat<eT>(aux_mem, 1, aux_length, copy_aux_mem, strict)
  {
  arma_extra_debug_sigprint();

  access::rw(Mat<eT>::vec_state) = 2;
  }



//! construct a row vector from a given auxiliary array
template<typename eT>
inline
Row<eT>::Row(const eT* aux_mem, const uword aux_length)
  : Mat<eT>(aux_mem, 1, aux_length)
  {
  arma_extra_debug_sigprint();

  access::rw(Mat<eT>::vec_state) = 2;
  }



template<typename eT>
template<typename T1, typename T2>
inline
Row<eT>::Row
  (
  const Base<typename Row<eT>::pod_type, T1>& A,
  const Base<typename Row<eT>::pod_type, T2>& B
  )
  {
  arma_extra_debug_sigprint();

  access::rw(Mat<eT>::vec_state) = 2;

  Mat<eT>::init(A,B);
  }



template<typename eT>
template<typename T1>
inline
Row<eT>::Row(const BaseCube<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  access::rw(Mat<eT>::vec_state) = 2;

  Mat<eT>::operator=(X);
  }



template<typename eT>
template<typename T1>
inline
Row<eT>&
Row<eT>::operator=(const BaseCube<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  Mat<eT>::operator=(X);

  return *this;
  }



template<typename eT>
inline
Row<eT>::Row(const subview_cube<eT>& X)
  {
  arma_extra_debug_sigprint();

  access::rw(Mat<eT>::vec_state) = 2;

  Mat<eT>::operator=(X);
  }



template<typename eT>
inline
Row<eT>&
Row<eT>::operator=(const subview_cube<eT>& X)
  {
  arma_extra_debug_sigprint();

  Mat<eT>::operator=(X);

  return *this;
  }



template<typename eT>
inline
mat_injector< Row<eT> >
Row<eT>::operator<<(const eT val)
  {
  return mat_injector< Row<eT> >(*this, val);
  }



template<typename eT>
arma_inline
const Op<Row<eT>,op_htrans>
Row<eT>::t() const
  {
  return Op<Row<eT>,op_htrans>(*this);
  }



template<typename eT>
arma_inline
const Op<Row<eT>,op_htrans>
Row<eT>::ht() const
  {
  return Op<Row<eT>,op_htrans>(*this);
  }



template<typename eT>
arma_inline
const Op<Row<eT>,op_strans>
Row<eT>::st() const
  {
  return Op<Row<eT>,op_strans>(*this);
  }



template<typename eT>
arma_inline
subview_row<eT>
Row<eT>::col(const uword in_col1)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (in_col1 >= Mat<eT>::n_cols), "Row::col(): indices out of bounds or incorrectly used");

  return subview_row<eT>(*this, 0, in_col1, 1);
  }



template<typename eT>
arma_inline
const subview_row<eT>
Row<eT>::col(const uword in_col1) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (in_col1 >= Mat<eT>::n_cols), "Row::col(): indices out of bounds or incorrectly used");

  return subview_row<eT>(*this, 0, in_col1, 1);
  }



template<typename eT>
arma_inline
subview_row<eT>
Row<eT>::cols(const uword in_col1, const uword in_col2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( ( (in_col1 > in_col2) || (in_col2 >= Mat<eT>::n_cols) ), "Row::cols(): indices out of bounds or incorrectly used");

  const uword subview_n_cols = in_col2 - in_col1 + 1;

  return subview_row<eT>(*this, 0, in_col1, subview_n_cols);
  }



template<typename eT>
arma_inline
const subview_row<eT>
Row<eT>::cols(const uword in_col1, const uword in_col2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( ( (in_col1 > in_col2) || (in_col2 >= Mat<eT>::n_cols) ), "Row::cols(): indices out of bounds or incorrectly used");

  const uword subview_n_cols = in_col2 - in_col1 + 1;

  return subview_row<eT>(*this, 0, in_col1, subview_n_cols);
  }



template<typename eT>
arma_inline
subview_row<eT>
Row<eT>::subvec(const uword in_col1, const uword in_col2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( ( (in_col1 > in_col2) || (in_col2 >= Mat<eT>::n_cols) ), "Row::subvec(): indices out of bounds or incorrectly used");

  const uword subview_n_cols = in_col2 - in_col1 + 1;

  return subview_row<eT>(*this, 0, in_col1, subview_n_cols);
  }



template<typename eT>
arma_inline
const subview_row<eT>
Row<eT>::subvec(const uword in_col1, const uword in_col2) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( ( (in_col1 > in_col2) || (in_col2 >= Mat<eT>::n_cols) ), "Row::subvec(): indices out of bounds or incorrectly used");

  const uword subview_n_cols = in_col2 - in_col1 + 1;

  return subview_row<eT>(*this, 0, in_col1, subview_n_cols);
  }



template<typename eT>
arma_inline
subview_row<eT>
Row<eT>::cols(const span& col_span)
  {
  arma_extra_debug_sigprint();

  return subvec(col_span);
  }



template<typename eT>
arma_inline
const subview_row<eT>
Row<eT>::cols(const span& col_span) const
  {
  arma_extra_debug_sigprint();

  return subvec(col_span);
  }



template<typename eT>
arma_inline
subview_row<eT>
Row<eT>::subvec(const span& col_span)
  {
  arma_extra_debug_sigprint();

  const bool col_all = col_span.whole;

  const uword local_n_cols = Mat<eT>::n_cols;

  const uword in_col1       = col_all ? 0            : col_span.a;
  const uword in_col2       =                          col_span.b;
  const uword subvec_n_cols = col_all ? local_n_cols : in_col2 - in_col1 + 1;

  arma_debug_check( ( col_all ? false : ((in_col1 > in_col2) || (in_col2 >= local_n_cols)) ), "Row::subvec(): indices out of bounds or incorrectly used");

  return subview_row<eT>(*this, 0, in_col1, subvec_n_cols);
  }



template<typename eT>
arma_inline
const subview_row<eT>
Row<eT>::subvec(const span& col_span) const
  {
  arma_extra_debug_sigprint();

  const bool col_all = col_span.whole;

  const uword local_n_cols = Mat<eT>::n_cols;

  const uword in_col1       = col_all ? 0            : col_span.a;
  const uword in_col2       =                          col_span.b;
  const uword subvec_n_cols = col_all ? local_n_cols : in_col2 - in_col1 + 1;

  arma_debug_check( ( col_all ? false : ((in_col1 > in_col2) || (in_col2 >= local_n_cols)) ), "Row::subvec(): indices out of bounds or incorrectly used");

  return subview_row<eT>(*this, 0, in_col1, subvec_n_cols);
  }



template<typename eT>
arma_inline
subview_row<eT>
Row<eT>::operator()(const span& col_span)
  {
  arma_extra_debug_sigprint();

  return subvec(col_span);
  }



template<typename eT>
arma_inline
const subview_row<eT>
Row<eT>::operator()(const span& col_span) const
  {
  arma_extra_debug_sigprint();

  return subvec(col_span);
  }



template<typename eT>
arma_inline
subview_row<eT>
Row<eT>::subvec(const uword start_col, const SizeMat& s)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (s.n_rows != 1), "Row::subvec(): given size does not specify a row vector" );

  arma_debug_check( ( (start_col >= Mat<eT>::n_cols) || ((start_col + s.n_cols) > Mat<eT>::n_cols) ), "Row::subvec(): size out of bounds" );

  return subview_row<eT>(*this, 0, start_col, s.n_cols);
  }



template<typename eT>
arma_inline
const subview_row<eT>
Row<eT>::subvec(const uword start_col, const SizeMat& s) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (s.n_rows != 1), "Row::subvec(): given size does not specify a row vector" );

  arma_debug_check( ( (start_col >= Mat<eT>::n_cols) || ((start_col + s.n_cols) > Mat<eT>::n_cols) ), "Row::subvec(): size out of bounds" );

  return subview_row<eT>(*this, 0, start_col, s.n_cols);
  }



template<typename eT>
arma_inline
subview_row<eT>
Row<eT>::head(const uword N)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > Mat<eT>::n_cols), "Row::head(): size out of bounds");

  return subview_row<eT>(*this, 0, 0, N);
  }



template<typename eT>
arma_inline
const subview_row<eT>
Row<eT>::head(const uword N) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > Mat<eT>::n_cols), "Row::head(): size out of bounds");

  return subview_row<eT>(*this, 0, 0, N);
  }



template<typename eT>
arma_inline
subview_row<eT>
Row<eT>::tail(const uword N)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > Mat<eT>::n_cols), "Row::tail(): size out of bounds");

  const uword start_col = Mat<eT>::n_cols - N;

  return subview_row<eT>(*this, 0, start_col, N);
  }



template<typename eT>
arma_inline
const subview_row<eT>
Row<eT>::tail(const uword N) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (N > Mat<eT>::n_cols), "Row::tail(): size out of bounds");

  const uword start_col = Mat<eT>::n_cols - N;

  return subview_row<eT>(*this, 0, start_col, N);
  }



template<typename eT>
arma_inline
subview_row<eT>
Row<eT>::head_cols(const uword N)
  {
  arma_extra_debug_sigprint();

  return (*this).head(N);
  }



template<typename eT>
arma_inline
const subview_row<eT>
Row<eT>::head_cols(const uword N) const
  {
  arma_extra_debug_sigprint();

  return (*this).head(N);
  }



template<typename eT>
arma_inline
subview_row<eT>
Row<eT>::tail_cols(const uword N)
  {
  arma_extra_debug_sigprint();

  return (*this).tail(N);
  }



template<typename eT>
arma_inline
const subview_row<eT>
Row<eT>::tail_cols(const uword N) const
  {
  arma_extra_debug_sigprint();

  return (*this).tail(N);
  }



//! remove specified columns
template<typename eT>
inline
void
Row<eT>::shed_col(const uword col_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( col_num >= Mat<eT>::n_cols, "Row::shed_col(): index out of bounds");

  shed_cols(col_num, col_num);
  }



//! remove specified columns
template<typename eT>
inline
void
Row<eT>::shed_cols(const uword in_col1, const uword in_col2)
  {
  arma_extra_debug_sigprint();

  arma_debug_check
    (
    (in_col1 > in_col2) || (in_col2 >= Mat<eT>::n_cols),
    "Row::shed_cols(): indices out of bounds or incorrectly used"
    );

  const uword n_keep_front = in_col1;
  const uword n_keep_back  = Mat<eT>::n_cols - (in_col2 + 1);

  Row<eT> X(n_keep_front + n_keep_back);

        eT* X_mem = X.memptr();
  const eT* t_mem = (*this).memptr();

  if(n_keep_front > 0)
    {
    arrayops::copy( X_mem, t_mem, n_keep_front );
    }

  if(n_keep_back > 0)
    {
    arrayops::copy( &(X_mem[n_keep_front]), &(t_mem[in_col2+1]), n_keep_back);
    }

  Mat<eT>::steal_mem(X);
  }



//! insert N cols at the specified col position,
//! optionally setting the elements of the inserted cols to zero
template<typename eT>
inline
void
Row<eT>::insert_cols(const uword col_num, const uword N, const bool set_to_zero)
  {
  arma_extra_debug_sigprint();

  const uword t_n_cols = Mat<eT>::n_cols;

  const uword A_n_cols = col_num;
  const uword B_n_cols = t_n_cols - col_num;

  // insertion at col_num == n_cols is in effect an append operation
  arma_debug_check( (col_num > t_n_cols), "Row::insert_cols(): index out of bounds");

  if(N > 0)
    {
    Row<eT> out(t_n_cols + N);

          eT* out_mem = out.memptr();
    const eT*   t_mem = (*this).memptr();

    if(A_n_cols > 0)
      {
      arrayops::copy( out_mem, t_mem, A_n_cols );
      }

    if(B_n_cols > 0)
      {
      arrayops::copy( &(out_mem[col_num + N]), &(t_mem[col_num]), B_n_cols );
      }

    if(set_to_zero == true)
      {
      arrayops::inplace_set( &(out_mem[col_num]), eT(0), N );
      }

    Mat<eT>::steal_mem(out);
    }
  }



//! insert the given object at the specified col position;
//! the given object must have one row
template<typename eT>
template<typename T1>
inline
void
Row<eT>::insert_cols(const uword col_num, const Base<eT,T1>& X)
  {
  arma_extra_debug_sigprint();

  Mat<eT>::insert_cols(col_num, X);
  }



template<typename eT>
arma_inline
arma_warn_unused
eT&
Row<eT>::at(const uword i)
  {
  return access::rw(Mat<eT>::mem[i]);
  }



template<typename eT>
arma_inline
arma_warn_unused
const eT&
Row<eT>::at(const uword i) const
  {
  return Mat<eT>::mem[i];
  }



template<typename eT>
arma_inline
arma_warn_unused
eT&
Row<eT>::at(const uword, const uword in_col)
  {
  return access::rw( Mat<eT>::mem[in_col] );
  }



template<typename eT>
arma_inline
arma_warn_unused
const eT&
Row<eT>::at(const uword, const uword in_col) const
  {
  return Mat<eT>::mem[in_col];
  }



template<typename eT>
inline
typename Row<eT>::row_iterator
Row<eT>::begin_row(const uword row_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (row_num >= Mat<eT>::n_rows), "Row::begin_row(): index out of bounds");

  return Mat<eT>::memptr();
  }



template<typename eT>
inline
typename Row<eT>::const_row_iterator
Row<eT>::begin_row(const uword row_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (row_num >= Mat<eT>::n_rows), "Row::begin_row(): index out of bounds");

  return Mat<eT>::memptr();
  }



template<typename eT>
inline
typename Row<eT>::row_iterator
Row<eT>::end_row(const uword row_num)
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (row_num >= Mat<eT>::n_rows), "Row::end_row(): index out of bounds");

  return Mat<eT>::memptr() + Mat<eT>::n_cols;
  }



template<typename eT>
inline
typename Row<eT>::const_row_iterator
Row<eT>::end_row(const uword row_num) const
  {
  arma_extra_debug_sigprint();

  arma_debug_check( (row_num >= Mat<eT>::n_rows), "Row::end_row(): index out of bounds");

  return Mat<eT>::memptr() + Mat<eT>::n_cols;
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
Row<eT>::fixed<fixed_n_elem>::fixed()
  : Row<eT>( arma_fixed_indicator(), fixed_n_elem, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
Row<eT>::fixed<fixed_n_elem>::fixed(const fixed<fixed_n_elem>& X)
  : Row<eT>( arma_fixed_indicator(), fixed_n_elem, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);

        eT* dest = (use_extra) ?   mem_local_extra : Mat<eT>::mem_local;
  const eT* src  = (use_extra) ? X.mem_local_extra :        X.mem_local;

  arrayops::copy( dest, src, fixed_n_elem );
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
Row<eT>::fixed<fixed_n_elem>::fixed(const subview_cube<eT>& X)
  : Row<eT>( arma_fixed_indicator(), fixed_n_elem, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);

  Row<eT>::operator=(X);
  }



template<typename eT>
template<uword fixed_n_elem>
template<typename fill_type>
inline
Row<eT>::fixed<fixed_n_elem>::fixed(const fill::fill_class<fill_type>&)
  : Row<eT>( arma_fixed_indicator(), fixed_n_elem, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);

  if(is_same_type<fill_type, fill::fill_zeros>::yes)  (*this).zeros();
  if(is_same_type<fill_type, fill::fill_ones >::yes)  (*this).ones();
  if(is_same_type<fill_type, fill::fill_eye  >::yes)  (*this).eye();
  if(is_same_type<fill_type, fill::fill_randu>::yes)  (*this).randu();
  if(is_same_type<fill_type, fill::fill_randn>::yes)  (*this).randn();
  }



template<typename eT>
template<uword fixed_n_elem>
template<typename T1>
arma_inline
Row<eT>::fixed<fixed_n_elem>::fixed(const Base<eT,T1>& A)
  : Row<eT>( arma_fixed_indicator(), fixed_n_elem, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);

  Row<eT>::operator=(A.get_ref());
  }



template<typename eT>
template<uword fixed_n_elem>
template<typename T1, typename T2>
arma_inline
Row<eT>::fixed<fixed_n_elem>::fixed(const Base<pod_type,T1>& A, const Base<pod_type,T2>& B)
  : Row<eT>( arma_fixed_indicator(), fixed_n_elem, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);

  Row<eT>::init(A,B);
  }



template<typename eT>
template<uword fixed_n_elem>
inline
Row<eT>::fixed<fixed_n_elem>::fixed(const eT* aux_mem)
  : Row<eT>( arma_fixed_indicator(), fixed_n_elem, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);

  eT* dest = (use_extra) ? mem_local_extra : Mat<eT>::mem_local;

  arrayops::copy( dest, aux_mem, fixed_n_elem );
  }



template<typename eT>
template<uword fixed_n_elem>
inline
Row<eT>::fixed<fixed_n_elem>::fixed(const char* text)
  : Row<eT>( arma_fixed_indicator(), fixed_n_elem, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);

  Row<eT>::operator=(text);
  }



template<typename eT>
template<uword fixed_n_elem>
inline
Row<eT>::fixed<fixed_n_elem>::fixed(const std::string& text)
  : Row<eT>( arma_fixed_indicator(), fixed_n_elem, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
  {
  arma_extra_debug_sigprint_this(this);

  Row<eT>::operator=(text);
  }



template<typename eT>
template<uword fixed_n_elem>
template<typename T1>
Row<eT>&
Row<eT>::fixed<fixed_n_elem>::operator=(const Base<eT,T1>& A)
  {
  arma_extra_debug_sigprint();

  Row<eT>::operator=(A.get_ref());

  return *this;
  }



template<typename eT>
template<uword fixed_n_elem>
Row<eT>&
Row<eT>::fixed<fixed_n_elem>::operator=(const eT val)
  {
  arma_extra_debug_sigprint();

  Row<eT>::operator=(val);

  return *this;
  }



template<typename eT>
template<uword fixed_n_elem>
Row<eT>&
Row<eT>::fixed<fixed_n_elem>::operator=(const char* text)
  {
  arma_extra_debug_sigprint();

  Row<eT>::operator=(text);

  return *this;
  }



template<typename eT>
template<uword fixed_n_elem>
Row<eT>&
Row<eT>::fixed<fixed_n_elem>::operator=(const std::string& text)
  {
  arma_extra_debug_sigprint();

  Row<eT>::operator=(text);

  return *this;
  }



template<typename eT>
template<uword fixed_n_elem>
Row<eT>&
Row<eT>::fixed<fixed_n_elem>::operator=(const subview_cube<eT>& X)
  {
  arma_extra_debug_sigprint();

  Row<eT>::operator=(X);

  return *this;
  }



#if defined(ARMA_USE_CXX11)

  template<typename eT>
  template<uword fixed_n_elem>
  inline
  Row<eT>::fixed<fixed_n_elem>::fixed(const std::initializer_list<eT>& list)
    : Row<eT>( arma_fixed_indicator(), fixed_n_elem, ((use_extra) ? mem_local_extra : Mat<eT>::mem_local) )
    {
    arma_extra_debug_sigprint_this(this);

    (*this).operator=(list);
    }



  template<typename eT>
  template<uword fixed_n_elem>
  inline
  Row<eT>&
  Row<eT>::fixed<fixed_n_elem>::operator=(const std::initializer_list<eT>& list)
    {
    arma_extra_debug_sigprint();

    const uword N = uword(list.size());

    arma_debug_check( (N > fixed_n_elem), "Row::fixed: initialiser list is too long" );

    eT* this_mem = (*this).memptr();

    arrayops::copy( this_mem, list.begin(), N );

    for(uword iq=N; iq < fixed_n_elem; ++iq) { this_mem[iq] = eT(0); }

    return *this;
    }

#endif



template<typename eT>
template<uword fixed_n_elem>
arma_inline
Row<eT>&
Row<eT>::fixed<fixed_n_elem>::operator=(const fixed<fixed_n_elem>& X)
  {
  arma_extra_debug_sigprint();

  if(this != &X)
    {
          eT* dest = (use_extra) ?   mem_local_extra : Mat<eT>::mem_local;
    const eT* src  = (use_extra) ? X.mem_local_extra :        X.mem_local;

    arrayops::copy( dest, src, fixed_n_elem );
    }

  return *this;
  }



#if defined(ARMA_GOOD_COMPILER)

  template<typename eT>
  template<uword fixed_n_elem>
  template<typename T1, typename eop_type>
  inline
  Row<eT>&
  Row<eT>::fixed<fixed_n_elem>::operator=(const eOp<T1, eop_type>& X)
    {
    arma_extra_debug_sigprint();

    arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));

    const bool bad_alias = (eOp<T1, eop_type>::proxy_type::has_subview  &&  X.P.is_alias(*this));

    if(bad_alias == false)
      {
      arma_debug_assert_same_size(uword(1), fixed_n_elem, X.get_n_rows(), X.get_n_cols(), "Row::fixed::operator=");

      eop_type::apply(*this, X);
      }
    else
      {
      arma_extra_debug_print("bad_alias = true");

      Row<eT> tmp(X);

      (*this) = tmp;
      }

    return *this;
    }



  template<typename eT>
  template<uword fixed_n_elem>
  template<typename T1, typename T2, typename eglue_type>
  inline
  Row<eT>&
  Row<eT>::fixed<fixed_n_elem>::operator=(const eGlue<T1, T2, eglue_type>& X)
    {
    arma_extra_debug_sigprint();

    arma_type_check(( is_same_type< eT, typename T1::elem_type >::no ));
    arma_type_check(( is_same_type< eT, typename T2::elem_type >::no ));

    const bool bad_alias =
      (
      (eGlue<T1, T2, eglue_type>::proxy1_type::has_subview  &&  X.P1.is_alias(*this))
      ||
      (eGlue<T1, T2, eglue_type>::proxy2_type::has_subview  &&  X.P2.is_alias(*this))
      );

    if(bad_alias == false)
      {
      arma_debug_assert_same_size(uword(1), fixed_n_elem, X.get_n_rows(), X.get_n_cols(), "Row::fixed::operator=");

      eglue_type::apply(*this, X);
      }
    else
      {
      arma_extra_debug_print("bad_alias = true");

      Row<eT> tmp(X);

      (*this) = tmp;
      }

    return *this;
    }

#endif



template<typename eT>
template<uword fixed_n_elem>
arma_inline
const Op< typename Row<eT>::template fixed<fixed_n_elem>::Row_fixed_type, op_htrans >
Row<eT>::fixed<fixed_n_elem>::t() const
  {
  return Op< typename Row<eT>::template fixed<fixed_n_elem>::Row_fixed_type, op_htrans >(*this);
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
const Op< typename Row<eT>::template fixed<fixed_n_elem>::Row_fixed_type, op_htrans >
Row<eT>::fixed<fixed_n_elem>::ht() const
  {
  return Op< typename Row<eT>::template fixed<fixed_n_elem>::Row_fixed_type, op_htrans >(*this);
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
const Op< typename Row<eT>::template fixed<fixed_n_elem>::Row_fixed_type, op_strans >
Row<eT>::fixed<fixed_n_elem>::st() const
  {
  return Op< typename Row<eT>::template fixed<fixed_n_elem>::Row_fixed_type, op_strans >(*this);
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
arma_warn_unused
const eT&
Row<eT>::fixed<fixed_n_elem>::at_alt(const uword ii) const
  {
  #if defined(ARMA_HAVE_ALIGNED_ATTRIBUTE)

    return (use_extra) ? mem_local_extra[ii] : Mat<eT>::mem_local[ii];

  #else
    const eT* mem_aligned = (use_extra) ? mem_local_extra : Mat<eT>::mem_local;

    memory::mark_as_aligned(mem_aligned);

    return mem_aligned[ii];
  #endif
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
arma_warn_unused
eT&
Row<eT>::fixed<fixed_n_elem>::operator[] (const uword ii)
  {
  return (use_extra) ? mem_local_extra[ii] : Mat<eT>::mem_local[ii];
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
arma_warn_unused
const eT&
Row<eT>::fixed<fixed_n_elem>::operator[] (const uword ii) const
  {
  return (use_extra) ? mem_local_extra[ii] : Mat<eT>::mem_local[ii];
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
arma_warn_unused
eT&
Row<eT>::fixed<fixed_n_elem>::at(const uword ii)
  {
  return (use_extra) ? mem_local_extra[ii] : Mat<eT>::mem_local[ii];
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
arma_warn_unused
const eT&
Row<eT>::fixed<fixed_n_elem>::at(const uword ii) const
  {
  return (use_extra) ? mem_local_extra[ii] : Mat<eT>::mem_local[ii];
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
arma_warn_unused
eT&
Row<eT>::fixed<fixed_n_elem>::operator() (const uword ii)
  {
  arma_debug_check( (ii >= fixed_n_elem), "Row::operator(): index out of bounds");

  return (use_extra) ? mem_local_extra[ii] : Mat<eT>::mem_local[ii];
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
arma_warn_unused
const eT&
Row<eT>::fixed<fixed_n_elem>::operator() (const uword ii) const
  {
  arma_debug_check( (ii >= fixed_n_elem), "Row::operator(): index out of bounds");

  return (use_extra) ? mem_local_extra[ii] : Mat<eT>::mem_local[ii];
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
arma_warn_unused
eT&
Row<eT>::fixed<fixed_n_elem>::at(const uword, const uword in_col)
  {
  return (use_extra) ? mem_local_extra[in_col] : Mat<eT>::mem_local[in_col];
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
arma_warn_unused
const eT&
Row<eT>::fixed<fixed_n_elem>::at(const uword, const uword in_col) const
  {
  return (use_extra) ? mem_local_extra[in_col] : Mat<eT>::mem_local[in_col];
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
arma_warn_unused
eT&
Row<eT>::fixed<fixed_n_elem>::operator() (const uword in_row, const uword in_col)
  {
  arma_debug_check( ((in_row > 0) || (in_col >= fixed_n_elem)), "Row::operator(): index out of bounds" );

  return (use_extra) ? mem_local_extra[in_col] : Mat<eT>::mem_local[in_col];
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
arma_warn_unused
const eT&
Row<eT>::fixed<fixed_n_elem>::operator() (const uword in_row, const uword in_col) const
  {
  arma_debug_check( ((in_row > 0) || (in_col >= fixed_n_elem)), "Row::operator(): index out of bounds" );

  return (use_extra) ? mem_local_extra[in_col] : Mat<eT>::mem_local[in_col];
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
arma_warn_unused
eT*
Row<eT>::fixed<fixed_n_elem>::memptr()
  {
  return (use_extra) ? mem_local_extra : Mat<eT>::mem_local;
  }



template<typename eT>
template<uword fixed_n_elem>
arma_inline
arma_warn_unused
const eT*
Row<eT>::fixed<fixed_n_elem>::memptr() const
  {
  return (use_extra) ? mem_local_extra : Mat<eT>::mem_local;
  }



template<typename eT>
template<uword fixed_n_elem>
arma_hot
inline
const Row<eT>&
Row<eT>::fixed<fixed_n_elem>::fill(const eT val)
  {
  arma_extra_debug_sigprint();

  eT* mem_use = (use_extra) ? &(mem_local_extra[0]) : &(Mat<eT>::mem_local[0]);

  arrayops::inplace_set_fixed<eT,fixed_n_elem>( mem_use, val );

  return *this;
  }



template<typename eT>
template<uword fixed_n_elem>
arma_hot
inline
const Row<eT>&
Row<eT>::fixed<fixed_n_elem>::zeros()
  {
  arma_extra_debug_sigprint();

  eT* mem_use = (use_extra) ? &(mem_local_extra[0]) : &(Mat<eT>::mem_local[0]);

  arrayops::inplace_set_fixed<eT,fixed_n_elem>( mem_use, eT(0) );

  return *this;
  }



template<typename eT>
template<uword fixed_n_elem>
arma_hot
inline
const Row<eT>&
Row<eT>::fixed<fixed_n_elem>::ones()
  {
  arma_extra_debug_sigprint();

  eT* mem_use = (use_extra) ? &(mem_local_extra[0]) : &(Mat<eT>::mem_local[0]);

  arrayops::inplace_set_fixed<eT,fixed_n_elem>( mem_use, eT(1) );

  return *this;
  }



template<typename eT>
inline
Row<eT>::Row(const arma_fixed_indicator&, const uword in_n_elem, const eT* in_mem)
  : Mat<eT>(arma_fixed_indicator(), 1, in_n_elem, 2, in_mem)
  {
  arma_extra_debug_sigprint_this(this);
  }



#ifdef ARMA_EXTRA_ROW_MEAT
  #include ARMA_INCFILE_WRAP(ARMA_EXTRA_ROW_MEAT)
#endif



//! @}
