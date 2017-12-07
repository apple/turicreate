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


//! \addtogroup glue_affmul
//! @{



template<typename T1, typename T2>
inline
void
glue_affmul::apply(Mat<typename T1::elem_type>& out, const Glue<T1,T2,glue_affmul>& X)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  const quasi_unwrap<T1> U1(X.A);
  const quasi_unwrap<T2> U2(X.B);

  const bool is_alias = (U1.is_alias(out) || U2.is_alias(out));

  if(is_alias == false)
    {
    glue_affmul::apply_noalias(out, U1.M, U2.M);
    }
  else
    {
    Mat<eT> tmp;

    glue_affmul::apply_noalias(tmp, U1.M, U2.M);

    out.steal_mem(tmp);
    }
  }



template<typename T1, typename T2>
inline
void
glue_affmul::apply_noalias(Mat<typename T1::elem_type>& out, const T1& A, const T2& B)
  {
  arma_extra_debug_sigprint();

  const uword A_n_cols = A.n_cols;
  const uword A_n_rows = A.n_rows;
  const uword B_n_rows = B.n_rows;

  arma_debug_check( (A_n_cols != B_n_rows+1), "affmul(): size mismatch" );

  if(A_n_rows == A_n_cols)
    {
    glue_affmul::apply_noalias_square(out, A, B);
    }
  else
  if(A_n_rows == B_n_rows)
    {
    glue_affmul::apply_noalias_rectangle(out, A, B);
    }
  else
    {
    glue_affmul::apply_noalias_generic(out, A, B);
    }
  }



template<typename T1, typename T2>
inline
void
glue_affmul::apply_noalias_square(Mat<typename T1::elem_type>& out, const T1& A, const T2& B)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  // assuming that A is square sized, and A.n_cols = B.n_rows+1

  const uword N        = A.n_rows;
  const uword B_n_cols = B.n_cols;

  out.set_size(N, B_n_cols);

  if(out.n_elem == 0)  { return; }

  const eT* A_mem = A.memptr();

  switch(N)
    {
    case 0:
      break;

    case 1:  // A is 1x1
      out.fill(A_mem[0]);
      break;

    case 2:  // A is 2x2
      {
      if(B_n_cols == 1)
        {
        const eT*   B_mem =   B.memptr();
              eT* out_mem = out.memptr();

        const eT x = B_mem[0];

        out_mem[0] = A_mem[0]*x + A_mem[2];
        out_mem[1] = A_mem[1]*x + A_mem[3];
        }
      else
      for(uword col=0; col < B_n_cols; ++col)
        {
        const eT*   B_mem =   B.colptr(col);
              eT* out_mem = out.colptr(col);

        const eT x = B_mem[0];

        out_mem[0] = A_mem[0]*x + A_mem[2];
        out_mem[1] = A_mem[1]*x + A_mem[3];
        }
      }
      break;

    case 3:  // A is 3x3
      {
      if(B_n_cols == 1)
        {
        const eT*   B_mem =   B.memptr();
              eT* out_mem = out.memptr();

        const eT x = B_mem[0];
        const eT y = B_mem[1];

        out_mem[0] = A_mem[0]*x + A_mem[3]*y + A_mem[6];
        out_mem[1] = A_mem[1]*x + A_mem[4]*y + A_mem[7];
        out_mem[2] = A_mem[2]*x + A_mem[5]*y + A_mem[8];
        }
      else
      for(uword col=0; col < B_n_cols; ++col)
        {
        const eT*   B_mem =   B.colptr(col);
              eT* out_mem = out.colptr(col);

        const eT x = B_mem[0];
        const eT y = B_mem[1];

        out_mem[0] = A_mem[0]*x + A_mem[3]*y + A_mem[6];
        out_mem[1] = A_mem[1]*x + A_mem[4]*y + A_mem[7];
        out_mem[2] = A_mem[2]*x + A_mem[5]*y + A_mem[8];
        }
      }
      break;

    case 4:  // A is 4x4
      {
      if(B_n_cols == 1)
        {
        const eT*   B_mem =   B.memptr();
              eT* out_mem = out.memptr();

        const eT x = B_mem[0];
        const eT y = B_mem[1];
        const eT z = B_mem[2];

        out_mem[0] = A_mem[ 0]*x + A_mem[ 4]*y + A_mem[ 8]*z + A_mem[12];
        out_mem[1] = A_mem[ 1]*x + A_mem[ 5]*y + A_mem[ 9]*z + A_mem[13];
        out_mem[2] = A_mem[ 2]*x + A_mem[ 6]*y + A_mem[10]*z + A_mem[14];
        out_mem[3] = A_mem[ 3]*x + A_mem[ 7]*y + A_mem[11]*z + A_mem[15];
        }
      else
      for(uword col=0; col < B_n_cols; ++col)
        {
        const eT*   B_mem =   B.colptr(col);
              eT* out_mem = out.colptr(col);

        const eT x = B_mem[0];
        const eT y = B_mem[1];
        const eT z = B_mem[2];

        out_mem[0] = A_mem[ 0]*x + A_mem[ 4]*y + A_mem[ 8]*z + A_mem[12];
        out_mem[1] = A_mem[ 1]*x + A_mem[ 5]*y + A_mem[ 9]*z + A_mem[13];
        out_mem[2] = A_mem[ 2]*x + A_mem[ 6]*y + A_mem[10]*z + A_mem[14];
        out_mem[3] = A_mem[ 3]*x + A_mem[ 7]*y + A_mem[11]*z + A_mem[15];
        }
      }
      break;

    case 5:  // A is 5x5
      {
      if(B_n_cols == 1)
        {
        const eT*   B_mem =   B.memptr();
              eT* out_mem = out.memptr();

        const eT x = B_mem[0];
        const eT y = B_mem[1];
        const eT z = B_mem[2];
        const eT w = B_mem[3];

        out_mem[0] = A_mem[ 0]*x + A_mem[ 5]*y + A_mem[10]*z + A_mem[15]*w + A_mem[20];
        out_mem[1] = A_mem[ 1]*x + A_mem[ 6]*y + A_mem[11]*z + A_mem[16]*w + A_mem[21];
        out_mem[2] = A_mem[ 2]*x + A_mem[ 7]*y + A_mem[12]*z + A_mem[17]*w + A_mem[22];
        out_mem[3] = A_mem[ 3]*x + A_mem[ 8]*y + A_mem[13]*z + A_mem[18]*w + A_mem[23];
        out_mem[4] = A_mem[ 4]*x + A_mem[ 9]*y + A_mem[14]*z + A_mem[19]*w + A_mem[24];
        }
      else
      for(uword col=0; col < B_n_cols; ++col)
        {
        const eT*   B_mem =   B.colptr(col);
              eT* out_mem = out.colptr(col);

        const eT x = B_mem[0];
        const eT y = B_mem[1];
        const eT z = B_mem[2];
        const eT w = B_mem[3];

        out_mem[0] = A_mem[ 0]*x + A_mem[ 5]*y + A_mem[10]*z + A_mem[15]*w + A_mem[20];
        out_mem[1] = A_mem[ 1]*x + A_mem[ 6]*y + A_mem[11]*z + A_mem[16]*w + A_mem[21];
        out_mem[2] = A_mem[ 2]*x + A_mem[ 7]*y + A_mem[12]*z + A_mem[17]*w + A_mem[22];
        out_mem[3] = A_mem[ 3]*x + A_mem[ 8]*y + A_mem[13]*z + A_mem[18]*w + A_mem[23];
        out_mem[4] = A_mem[ 4]*x + A_mem[ 9]*y + A_mem[14]*z + A_mem[19]*w + A_mem[24];
        }
      }
      break;

    default:
      {
      if(B_n_cols == 1)
        {
        Col<eT> tmp(N);
        eT*     tmp_mem = tmp.memptr();

        arrayops::copy(tmp_mem, B.memptr(), N-1);

        tmp_mem[N-1] = eT(1);

        out = A * tmp;
        }
      else
        {
        Mat<eT> tmp(N, B_n_cols);

        for(uword col=0; col < B_n_cols; ++col)
          {
          const eT*   B_mem =   B.colptr(col);
                eT* tmp_mem = tmp.colptr(col);

          arrayops::copy(tmp_mem, B_mem, N-1);

          tmp_mem[N-1] = eT(1);
          }

        out = A * tmp;
        }
      }
    }
  }



template<typename T1, typename T2>
inline
void
glue_affmul::apply_noalias_rectangle(Mat<typename T1::elem_type>& out, const T1& A, const T2& B)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  // assuming that A.n_rows = A.n_cols-1, and A.n_cols = B.n_rows+1
  // (A and B have the same number of rows)

  const uword A_n_rows = A.n_rows;
  const uword B_n_cols = B.n_cols;

  out.set_size(A_n_rows, B_n_cols);

  if(out.n_elem == 0)  { return; }

  const eT* A_mem = A.memptr();

  switch(A_n_rows)
    {
    case 0:
      break;

    case 1:  // A is 1x2
      {
      if(B_n_cols == 1)
        {
        const eT*   B_mem =   B.memptr();
              eT* out_mem = out.memptr();

        const eT x = B_mem[0];

        out_mem[0] = A_mem[0]*x + A_mem[1];
        }
      else
      for(uword col=0; col < B_n_cols; ++col)
        {
        const eT*   B_mem =   B.colptr(col);
              eT* out_mem = out.colptr(col);

        const eT x = B_mem[0];

        out_mem[0] = A_mem[0]*x + A_mem[1];
        }
      }
      break;

    case 2:  // A is 2x3
      {
      if(B_n_cols == 1)
        {
        const eT*   B_mem =   B.memptr();
              eT* out_mem = out.memptr();

        const eT x = B_mem[0];
        const eT y = B_mem[1];

        out_mem[0] = A_mem[0]*x + A_mem[2]*y + A_mem[4];
        out_mem[1] = A_mem[1]*x + A_mem[3]*y + A_mem[5];
        }
      else
      for(uword col=0; col < B_n_cols; ++col)
        {
        const eT*   B_mem =   B.colptr(col);
              eT* out_mem = out.colptr(col);

        const eT x = B_mem[0];
        const eT y = B_mem[1];

        out_mem[0] = A_mem[0]*x + A_mem[2]*y + A_mem[4];
        out_mem[1] = A_mem[1]*x + A_mem[3]*y + A_mem[5];
        }
      }
      break;

    case 3:  // A is 3x4
      {
      if(B_n_cols == 1)
        {
        const eT*   B_mem =   B.memptr();
              eT* out_mem = out.memptr();

        const eT x = B_mem[0];
        const eT y = B_mem[1];
        const eT z = B_mem[2];

        out_mem[0] = A_mem[ 0]*x + A_mem[ 3]*y + A_mem[ 6]*z + A_mem[ 9];
        out_mem[1] = A_mem[ 1]*x + A_mem[ 4]*y + A_mem[ 7]*z + A_mem[10];
        out_mem[2] = A_mem[ 2]*x + A_mem[ 5]*y + A_mem[ 8]*z + A_mem[11];
        }
      else
      for(uword col=0; col < B_n_cols; ++col)
        {
        const eT*   B_mem =   B.colptr(col);
              eT* out_mem = out.colptr(col);

        const eT x = B_mem[0];
        const eT y = B_mem[1];
        const eT z = B_mem[2];

        out_mem[0] = A_mem[ 0]*x + A_mem[ 3]*y + A_mem[ 6]*z + A_mem[ 9];
        out_mem[1] = A_mem[ 1]*x + A_mem[ 4]*y + A_mem[ 7]*z + A_mem[10];
        out_mem[2] = A_mem[ 2]*x + A_mem[ 5]*y + A_mem[ 8]*z + A_mem[11];
        }
      }
      break;

    case 4:  // A is 4x5
      {
      if(B_n_cols == 1)
        {
        const eT*   B_mem =   B.memptr();
              eT* out_mem = out.memptr();

        const eT x = B_mem[0];
        const eT y = B_mem[1];
        const eT z = B_mem[2];
        const eT w = B_mem[3];

        out_mem[0] = A_mem[ 0]*x + A_mem[ 4]*y + A_mem[ 8]*z + A_mem[12]*w + A_mem[16];
        out_mem[1] = A_mem[ 1]*x + A_mem[ 5]*y + A_mem[ 9]*z + A_mem[13]*w + A_mem[17];
        out_mem[2] = A_mem[ 2]*x + A_mem[ 6]*y + A_mem[10]*z + A_mem[14]*w + A_mem[18];
        out_mem[3] = A_mem[ 3]*x + A_mem[ 7]*y + A_mem[11]*z + A_mem[15]*w + A_mem[19];
        }
      else
      for(uword col=0; col < B_n_cols; ++col)
        {
        const eT*   B_mem =   B.colptr(col);
              eT* out_mem = out.colptr(col);

        const eT x = B_mem[0];
        const eT y = B_mem[1];
        const eT z = B_mem[2];
        const eT w = B_mem[3];

        out_mem[0] = A_mem[ 0]*x + A_mem[ 4]*y + A_mem[ 8]*z + A_mem[12]*w + A_mem[16];
        out_mem[1] = A_mem[ 1]*x + A_mem[ 5]*y + A_mem[ 9]*z + A_mem[13]*w + A_mem[17];
        out_mem[2] = A_mem[ 2]*x + A_mem[ 6]*y + A_mem[10]*z + A_mem[14]*w + A_mem[18];
        out_mem[3] = A_mem[ 3]*x + A_mem[ 7]*y + A_mem[11]*z + A_mem[15]*w + A_mem[19];
        }
      }
      break;

    default:
      {
      const uword A_n_cols = A.n_cols;

      if(B_n_cols == 1)
        {
        Col<eT> tmp(A_n_cols);
        eT*     tmp_mem = tmp.memptr();

        arrayops::copy(tmp_mem, B.memptr(), A_n_cols-1);

        tmp_mem[A_n_cols-1] = eT(1);

        out = A * tmp;
        }
      else
        {
        Mat<eT> tmp(A_n_cols, B_n_cols);

        for(uword col=0; col < B_n_cols; ++col)
          {
          const eT*   B_mem =   B.colptr(col);
                eT* tmp_mem = tmp.colptr(col);

          arrayops::copy(tmp_mem, B_mem, A_n_cols-1);

          tmp_mem[A_n_cols-1] = eT(1);
          }

        out = A * tmp;
        }
      }
    }
  }



template<typename T1, typename T2>
inline
void
glue_affmul::apply_noalias_generic(Mat<typename T1::elem_type>& out, const T1& A, const T2& B)
  {
  arma_extra_debug_sigprint();

  typedef typename T1::elem_type eT;

  // assuming that A.n_cols = B.n_rows+1

  const uword B_n_rows = B.n_rows;
  const uword B_n_cols = B.n_cols;

  Mat<eT> tmp(B_n_rows+1, B_n_cols);

  for(uword col=0; col < B_n_cols; ++col)
    {
    const eT*   B_mem =   B.colptr(col);
          eT* tmp_mem = tmp.colptr(col);

    arrayops::copy(tmp_mem, B_mem, B_n_rows);

    tmp_mem[B_n_rows] = eT(1);
    }

  out = A * tmp;
  }



//! @}
