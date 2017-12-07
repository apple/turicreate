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


//! \addtogroup arma_ostream
//! @{



inline
arma_ostream_state::arma_ostream_state(const std::ostream& o)
  : orig_flags    (o.flags())
  , orig_precision(o.precision())
  , orig_width    (o.width())
  , orig_fill     (o.fill())
  {
  }



inline
void
arma_ostream_state::restore(std::ostream& o) const
  {
  o.flags    (orig_flags);
  o.precision(orig_precision);
  o.width    (orig_width);
  o.fill     (orig_fill);
  }



//
//



template<typename eT>
inline
std::streamsize
arma_ostream::modify_stream(std::ostream& o, const eT* data, const uword n_elem)
  {
  o.unsetf(ios::showbase);
  o.unsetf(ios::uppercase);
  o.unsetf(ios::showpos);

  o.fill(' ');

  std::streamsize cell_width;

  bool use_layout_B = false;
  bool use_layout_C = false;
  bool use_layout_D = false;

  for(uword i=0; i<n_elem; ++i)
    {
    const eT val = data[i];

    if(
      ( cond_rel< (sizeof(eT) > 4) && (is_same_type<uword,eT>::yes || is_same_type<sword,eT>::yes) >::geq(val, eT(+10000000000)) )
      ||
      ( cond_rel< (sizeof(eT) > 4) &&  is_same_type<sword,eT>::yes                                 >::leq(val, eT(-10000000000)) )
      )
      {
      use_layout_D = true;
      break;
      }

    if(
      ( val >= eT(+100) )
      ||
      //( (is_signed<eT>::value == true) && (val <= eT(-100)) ) ||
      //( (is_non_integral<eT>::value == true) && (val > eT(0)) && (val <= eT(+1e-4)) ) ||
      //( (is_non_integral<eT>::value == true) && (is_signed<eT>::value == true) && (val < eT(0)) && (val >= eT(-1e-4)) )
        (
        cond_rel< is_signed<eT>::value >::leq(val, eT(-100))
        )
      ||
        (
        cond_rel< is_non_integral<eT>::value >::gt(val,  eT(0))
        &&
        cond_rel< is_non_integral<eT>::value >::leq(val, eT(+1e-4))
        )
      ||
        (
        cond_rel< is_non_integral<eT>::value && is_signed<eT>::value >::lt(val, eT(0))
        &&
        cond_rel< is_non_integral<eT>::value && is_signed<eT>::value >::geq(val, eT(-1e-4))
        )
      )
      {
      use_layout_C = true;
      break;
      }

    if(
      // (val >= eT(+10)) || ( (is_signed<eT>::value == true) && (val <= eT(-10)) )
      (val >= eT(+10)) || ( cond_rel< is_signed<eT>::value >::leq(val, eT(-10)) )
      )
      {
      use_layout_B = true;
      }
    }

  if(use_layout_D)
    {
    o.setf(ios::scientific);
    o.setf(ios::right);
    o.unsetf(ios::fixed);
    o.precision(4);
    cell_width = 21;
    }
  else
  if(use_layout_C)
    {
    o.setf(ios::scientific);
    o.setf(ios::right);
    o.unsetf(ios::fixed);
    o.precision(4);
    cell_width = 13;
    }
  else
  if(use_layout_B)
    {
    o.unsetf(ios::scientific);
    o.setf(ios::right);
    o.setf(ios::fixed);
    o.precision(4);
    cell_width = 10;
    }
  else
    {
    o.unsetf(ios::scientific);
    o.setf(ios::right);
    o.setf(ios::fixed);
    o.precision(4);
    cell_width = 9;
    }

  return cell_width;
  }



//! "better than nothing" settings for complex numbers
template<typename T>
inline
std::streamsize
arma_ostream::modify_stream(std::ostream& o, const std::complex<T>* data, const uword n_elem)
  {
  arma_ignore(data);
  arma_ignore(n_elem);

  o.unsetf(ios::showbase);
  o.unsetf(ios::uppercase);
  o.fill(' ');

  o.setf(ios::scientific);
  o.setf(ios::showpos);
  o.setf(ios::right);
  o.unsetf(ios::fixed);

  std::streamsize cell_width;

  o.precision(3);
  cell_width = 2 + 2*(1 + 3 + o.precision() + 5) + 1;

  return cell_width;
  }


template<typename eT>
inline
std::streamsize
arma_ostream::modify_stream(std::ostream& o, typename SpMat<eT>::const_iterator begin, const uword n_elem, const typename arma_not_cx<eT>::result* junk)
  {
  arma_extra_debug_sigprint();
  arma_ignore(junk);

  o.unsetf(ios::showbase);
  o.unsetf(ios::uppercase);
  o.unsetf(ios::showpos);

  o.fill(' ');

  std::streamsize cell_width;

  bool use_layout_B  = false;
  bool use_layout_C  = false;

  for(typename SpMat<eT>::const_iterator it = begin; it.pos() < n_elem; ++it)
    {
    const eT val = *it;

    if(
      val >= eT(+100) ||
      ( (is_signed<eT>::value == true) && (val <= eT(-100)) ) ||
      ( (is_non_integral<eT>::value == true) && (val > eT(0)) && (val <= eT(+1e-4)) ) ||
      ( (is_non_integral<eT>::value == true) && (is_signed<eT>::value == true) && (val < eT(0)) && (val >= eT(-1e-4)) )
      )
      {
      use_layout_C = true;
      break;
      }

    if(
      (val >= eT(+10)) || ( (is_signed<eT>::value == true) && (val <= eT(-10)) )
      )
      {
      use_layout_B = true;
      }
    }

  if(use_layout_C)
    {
    o.setf(ios::scientific);
    o.setf(ios::right);
    o.unsetf(ios::fixed);
    o.precision(4);
    cell_width = 13;
    }
  else
  if(use_layout_B)
    {
    o.unsetf(ios::scientific);
    o.setf(ios::right);
    o.setf(ios::fixed);
    o.precision(4);
    cell_width = 10;
    }
  else
    {
    o.unsetf(ios::scientific);
    o.setf(ios::right);
    o.setf(ios::fixed);
    o.precision(4);
    cell_width = 9;
    }

  return cell_width;
  }



//! "better than nothing" settings for complex numbers
template<typename T>
inline
std::streamsize
arma_ostream::modify_stream(std::ostream& o, typename SpMat<T>::const_iterator begin, const uword n_elem, const typename arma_cx_only<T>::result* junk)
  {
  arma_ignore(begin);
  arma_ignore(n_elem);
  arma_ignore(junk);

  o.unsetf(ios::showbase);
  o.unsetf(ios::uppercase);
  o.fill(' ');

  o.setf(ios::scientific);
  o.setf(ios::showpos);
  o.setf(ios::right);
  o.unsetf(ios::fixed);

  std::streamsize cell_width;

  o.precision(3);
  cell_width = 2 + 2*(1 + 3 + o.precision() + 5) + 1;

  return cell_width;
  }



template<typename eT>
inline
void
arma_ostream::print_elem_zero(std::ostream& o, const bool modify)
  {
  if(modify == true)
    {
    const ios::fmtflags   save_flags     = o.flags();
    const std::streamsize save_precision = o.precision();

    o.unsetf(ios::scientific);
    o.setf(ios::fixed);
    o.precision(0);

    o << eT(0);

    o.flags(save_flags);
    o.precision(save_precision);
    }
  else
    {
    o << eT(0);
    }
  }



//! Print an element to the specified stream
template<typename eT>
arma_inline
void
arma_ostream::print_elem(std::ostream& o, const eT& x, const bool modify)
  {
  if(is_signed<eT>::value)
    {
    typedef typename promote_type<eT, s16>::result promoted_eT;

    if(x != eT(0))
      {
      if(arma_isfinite(x))
        {
        o << promoted_eT(x);
        }
      else
        {
        o << ( arma_isinf(x) ? ((x <= eT(0)) ? "-inf" : "inf") : "nan" );
        }
      }
    else
      {
      arma_ostream::print_elem_zero<promoted_eT>(o, modify);
      }
    }
  else
    {
    typedef typename promote_type<eT, u16>::result promoted_eT;

    if(x != eT(0))
      {
      o << promoted_eT(x);
      }
    else
      {
      arma_ostream::print_elem_zero<promoted_eT>(o, modify);
      }
    }
  }



//! Print a complex element to the specified stream
template<typename T>
inline
void
arma_ostream::print_elem(std::ostream& o, const std::complex<T>& x, const bool modify)
  {
  if( (x.real() != T(0)) || (x.imag() != T(0)) || (modify == false) )
    {
    std::ostringstream ss;
    ss.flags(o.flags());
    //ss.imbue(o.getloc());
    ss.precision(o.precision());

    ss << '(';

    const T a = x.real();

    if(arma_isfinite(a))
      {
      ss << a;
      }
    else
      {
      ss << ( arma_isinf(a) ? ((a <= T(0)) ? "-inf" : "+inf") : "nan" );
      }

    ss << ',';

    const T b = x.imag();

    if(arma_isfinite(b))
      {
      ss << b;
      }
    else
      {
      ss << ( arma_isinf(b) ? ((b <= T(0)) ? "-inf" : "+inf") : "nan" );
      }

    ss << ')';

    o << ss.str();
    }
  else
    {
    o << "(0,0)";
    }
  }



//! Print a matrix to the specified stream
template<typename eT>
arma_cold
inline
void
arma_ostream::print(std::ostream& o, const Mat<eT>& m, const bool modify)
  {
  arma_extra_debug_sigprint();

  const arma_ostream_state stream_state(o);

  const std::streamsize cell_width = modify ? arma_ostream::modify_stream(o, m.memptr(), m.n_elem) : o.width();

  const uword m_n_rows = m.n_rows;
  const uword m_n_cols = m.n_cols;

  if(m.is_empty() == false)
    {
    if(m_n_cols > 0)
      {
      if(cell_width > 0)
        {
        for(uword row=0; row < m_n_rows; ++row)
          {
          for(uword col=0; col < m_n_cols; ++col)
            {
            // the cell width appears to be reset after each element is printed,
            // hence we need to restore it
            o.width(cell_width);
            arma_ostream::print_elem(o, m.at(row,col), modify);
            }

          o << '\n';
          }
        }
      else
        {
        for(uword row=0; row < m_n_rows; ++row)
          {
          for(uword col=0; col < m_n_cols-1; ++col)
            {
            arma_ostream::print_elem(o, m.at(row,col), modify);
            o << ' ';
            }

          arma_ostream::print_elem(o, m.at(row, m_n_cols-1), modify);
          o << '\n';
          }
        }
      }
    }
  else
    {
    o << "[matrix size: " << m_n_rows << 'x' << m_n_cols << "]\n";
    }

  o.flush();
  stream_state.restore(o);
  }



//! Print a cube to the specified stream
template<typename eT>
arma_cold
inline
void
arma_ostream::print(std::ostream& o, const Cube<eT>& x, const bool modify)
  {
  arma_extra_debug_sigprint();

  const arma_ostream_state stream_state(o);

  if(x.is_empty() == false)
    {
    for(uword slice=0; slice < x.n_slices; ++slice)
      {
      const Mat<eT> tmp(const_cast<eT*>(x.slice_memptr(slice)), x.n_rows, x.n_cols, false);

      o << "[cube slice " << slice << ']' << '\n';
      arma_ostream::print(o, tmp, modify);
      o << '\n';
      }
    }
  else
    {
    o << "[cube size: " << x.n_rows << 'x' << x.n_cols << 'x' << x.n_slices <<  "]\n";
    }

  stream_state.restore(o);
  }




//! Print a field to the specified stream
//! Assumes type oT can be printed, i.e. oT has std::ostream& operator<< (std::ostream&, const oT&)
template<typename oT>
arma_cold
inline
void
arma_ostream::print(std::ostream& o, const field<oT>& x)
  {
  arma_extra_debug_sigprint();

  const arma_ostream_state stream_state(o);

  const std::streamsize cell_width = o.width();

  const uword x_n_rows   = x.n_rows;
  const uword x_n_cols   = x.n_cols;
  const uword x_n_slices = x.n_slices;

  if(x.is_empty() == false)
    {
    if(x_n_slices == 1)
      {
      for(uword col=0; col<x_n_cols; ++col)
        {
        o << "[field column " << col << ']' << '\n';

        for(uword row=0; row<x_n_rows; ++row)
          {
          o.width(cell_width);
          o << x.at(row,col) << '\n';
          }

        o << '\n';
        }
      }
    else
      {
      for(uword slice=0; slice<x_n_slices; ++slice)
        {
        o << "[field slice " << slice << ']' << '\n';

        for(uword col=0; col<x_n_cols; ++col)
          {
          o << "[field column " << col << ']' << '\n';

          for(uword row=0; row<x_n_rows; ++row)
            {
            o.width(cell_width);
            o << x.at(row,col,slice) << '\n';
            }

          o << '\n';
          }

        o << '\n';
        }
      }
    }
  else
    {
    o << "[field size: " << x_n_rows << 'x' << x_n_cols << 'x' << x_n_slices << "]\n";
    }

  o.flush();
  stream_state.restore(o);
  }



//! Print a subfield to the specified stream
//! Assumes type oT can be printed, i.e. oT has std::ostream& operator<< (std::ostream&, const oT&)
template<typename oT>
arma_cold
inline
void
arma_ostream::print(std::ostream& o, const subview_field<oT>& x)
  {
  arma_extra_debug_sigprint();

  const arma_ostream_state stream_state(o);

  const std::streamsize cell_width = o.width();

  const uword x_n_rows   = x.n_rows;
  const uword x_n_cols   = x.n_cols;
  const uword x_n_slices = x.n_slices;

  if(x.is_empty() == false)
    {
    if(x_n_slices == 1)
      {
      for(uword col=0; col<x_n_cols; ++col)
        {
        o << "[field column " << col << ']' << '\n';
        for(uword row=0; row<x_n_rows; ++row)
          {
          o.width(cell_width);
          o << x.at(row,col) << '\n';
          }

        o << '\n';
        }
      }
    else
      {
      for(uword slice=0; slice<x_n_slices; ++slice)
        {
        o << "[field slice " << slice << ']' << '\n';

        for(uword col=0; col<x_n_cols; ++col)
          {
          o << "[field column " << col << ']' << '\n';

          for(uword row=0; row<x_n_rows; ++row)
            {
            o.width(cell_width);
            o << x.at(row,col,slice) << '\n';
            }

          o << '\n';
          }

        o << '\n';
        }
      }
    }
  else
    {
    o << "[field size: " << x_n_rows << 'x' << x_n_cols << 'x' << x_n_slices << "]\n";
    }

  o.flush();
  stream_state.restore(o);
  }



template<typename eT>
arma_cold
inline
void
arma_ostream::print_dense(std::ostream& o, const SpMat<eT>& m, const bool modify)
  {
  arma_extra_debug_sigprint();

  const arma_ostream_state stream_state(o);

  const uword m_n_rows = m.n_rows;
  const uword m_n_cols = m.n_cols;

  if(m.n_nonzero > 0)
    {
    const std::streamsize cell_width = modify ? modify_stream<eT>(o, m.begin(), m.n_nonzero) : o.width();

    typename SpMat<eT>::const_iterator begin = m.begin();

    if(m_n_cols > 0)
      {
      if(cell_width > 0)
        {
        // An efficient row_iterator would make this simpler and faster
        for(uword row=0; row < m_n_rows; ++row)
          {
          for(uword col=0; col < m_n_cols; ++col)
            {
            // the cell width appears to be reset after each element is printed,
            // hence we need to restore it
            o.width(cell_width);
            eT val = eT(0);
            for(typename SpMat<eT>::const_iterator it = begin; it.pos() < m.n_nonzero; ++it)
              {
              if(it.row() == row && it.col() == col)
                {
                val = *it;
                break;
                }
              }
            arma_ostream::print_elem(o,eT(val), modify);
            }

          o << '\n';
          }
        }
      else
        {
        // An efficient row_iterator would make this simpler and faster
        for(uword row=0; row < m_n_rows; ++row)
          {
          for(uword col=0; col < m_n_cols; ++col)
            {
            eT val = eT(0);
            for(typename SpMat<eT>::const_iterator it = begin; it.pos() < m.n_nonzero; ++it)
              {
              if(it.row() == row && it.col() == col)
                {
                val = *it;
                break;
                }
              }
            arma_ostream::print_elem(o,eT(val), modify);
            o << ' ';
            }

          o << '\n';
          }
        }
      }
    }
  else
    {
    if(m.n_elem == 0)
      {
      o << "[matrix size: " << m_n_rows << 'x' << m_n_cols << "]\n";
      }
    else
      {
      eT tmp[1];
      tmp[0] = eT(0);

      const std::streamsize cell_width = modify ? arma_ostream::modify_stream(o, &tmp[0], 1) : o.width();

      for(uword row=0; row < m_n_rows; ++row)
        {
        for(uword col=0; col < m_n_cols; ++col)
          {
          o.width(cell_width);

          arma_ostream::print_elem_zero<eT>(o, modify);

          o << ' ';
          }

        o << '\n';
        }
      }
    }

  o.flush();
  stream_state.restore(o);
  }



template<typename eT>
arma_cold
inline
void
arma_ostream::print(std::ostream& o, const SpMat<eT>& m, const bool modify)
  {
  arma_extra_debug_sigprint();

  const arma_ostream_state stream_state(o);

  o.unsetf(ios::showbase);
  o.unsetf(ios::uppercase);
  o.unsetf(ios::showpos);
  o.unsetf(ios::scientific);
  o.setf(ios::right);
  o.setf(ios::fixed);
  o.precision(2);

  const uword m_n_nonzero = m.n_nonzero;

  o << "[matrix size: " << m.n_rows << 'x' << m.n_cols << "; n_nonzero: " << m_n_nonzero
    << "; density: " << ((m.n_elem > 0) ? (double(m_n_nonzero) / double(m.n_elem) * double(100)) : double(0))
    << "%]\n\n";

  if(modify == false) { stream_state.restore(o); }

  if(m_n_nonzero > 0)
    {
    const std::streamsize cell_width = modify ? modify_stream<eT>(o, m.begin(), m_n_nonzero) : o.width();

    typename SpMat<eT>::const_iterator begin = m.begin();
    typename SpMat<eT>::const_iterator m_end = m.end();

    while(begin != m_end)
      {
      const uword row = begin.row();

      // TODO: change the maximum number of spaces before and after each location to be dependent on n_rows and n_cols

           if(row < 10)      { o << "     "; }
      else if(row < 100)     { o << "    ";  }
      else if(row < 1000)    { o << "   ";   }
      else if(row < 10000)   { o << "  ";    }
      else if(row < 100000)  { o << ' ';     }

      const uword col = begin.col();

      o << '(' << row << ", " << col << ") ";

           if(col < 10)      { o << "     "; }
      else if(col < 100)     { o << "    ";  }
      else if(col < 1000)    { o << "   ";   }
      else if(col < 10000)   { o << "  ";    }
      else if(col < 100000)  { o << ' ';     }

      if(cell_width > 0) { o.width(cell_width); }

      arma_ostream::print_elem(o, eT(*begin), modify);
      o << '\n';

      ++begin;
      }

    o << '\n';
    }

  o.flush();
  stream_state.restore(o);
  }



arma_cold
inline
void
arma_ostream::print(std::ostream& o, const SizeMat& S)
  {
  arma_extra_debug_sigprint();

  const arma_ostream_state stream_state(o);

  o.unsetf(ios::showbase);
  o.unsetf(ios::uppercase);
  o.unsetf(ios::showpos);

  o.setf(ios::fixed);

  o << S.n_rows << 'x' << S.n_cols;

  stream_state.restore(o);
  }



arma_cold
inline
void
arma_ostream::print(std::ostream& o, const SizeCube& S)
  {
  arma_extra_debug_sigprint();

  const arma_ostream_state stream_state(o);

  o.unsetf(ios::showbase);
  o.unsetf(ios::uppercase);
  o.unsetf(ios::showpos);

  o.setf(ios::fixed);

  o << S.n_rows << 'x' << S.n_cols << 'x' << S.n_slices;

  stream_state.restore(o);
  }



//! @}
