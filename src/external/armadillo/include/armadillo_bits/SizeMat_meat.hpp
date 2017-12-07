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


//! \addtogroup SizeMat
//! @{



inline
SizeMat::SizeMat(const uword in_n_rows, const uword in_n_cols)
  : n_rows(in_n_rows)
  , n_cols(in_n_cols)
  {
  arma_extra_debug_sigprint();
  }



inline
uword
SizeMat::operator[](const uword dim) const
  {
  if(dim == 0)  { return n_rows; }
  if(dim == 1)  { return n_cols; }

  return uword(1);
  }



inline
uword
SizeMat::operator()(const uword dim) const
  {
  if(dim == 0)  { return n_rows; }
  if(dim == 1)  { return n_cols; }

  arma_debug_check(true, "size(): index out of bounds");

  return uword(1);
  }



inline
bool
SizeMat::operator==(const SizeMat& s) const
  {
  if(n_rows != s.n_rows)  { return false; }

  if(n_cols != s.n_cols)  { return false; }

  return true;
  }



inline
bool
SizeMat::operator!=(const SizeMat& s) const
  {
  if(n_rows != s.n_rows)  { return true; }

  if(n_cols != s.n_cols)  { return true; }

  return false;
  }



inline
SizeMat
SizeMat::operator+(const SizeMat& s) const
  {
  return SizeMat( (n_rows + s.n_rows), (n_cols + s.n_cols) );
  }



inline
SizeMat
SizeMat::operator-(const SizeMat& s) const
  {
  const uword out_n_rows = (n_rows > s.n_rows) ? (n_rows - s.n_rows) : uword(0);
  const uword out_n_cols = (n_cols > s.n_cols) ? (n_cols - s.n_cols) : uword(0);

  return SizeMat(out_n_rows, out_n_cols);
  }



inline
SizeMat
SizeMat::operator+(const uword val) const
  {
  return SizeMat( (n_rows + val), (n_cols + val) );
  }



inline
SizeMat
SizeMat::operator-(const uword val) const
  {
  const uword out_n_rows = (n_rows > val) ? (n_rows - val) : uword(0);
  const uword out_n_cols = (n_cols > val) ? (n_cols - val) : uword(0);

  return SizeMat(out_n_rows, out_n_cols);
  }



inline
SizeMat
SizeMat::operator*(const uword val) const
  {
  return SizeMat( (n_rows * val), (n_cols * val) );
  }



inline
SizeMat
SizeMat::operator/(const uword val) const
  {
  return SizeMat( (n_rows / val), (n_cols / val) );
  }



//! @}
