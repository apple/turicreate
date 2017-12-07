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


//! \addtogroup SizeCube
//! @{



class SizeCube
  {
  public:

  const uword n_rows;
  const uword n_cols;
  const uword n_slices;

  inline explicit SizeCube(const uword in_n_rows, const uword in_n_cols, const uword in_n_slices);

  inline uword operator[](const uword dim) const;
  inline uword operator()(const uword dim) const;

  inline bool operator==(const SizeCube& s) const;
  inline bool operator!=(const SizeCube& s) const;

  inline SizeCube operator+(const SizeCube& s) const;
  inline SizeCube operator-(const SizeCube& s) const;

  inline SizeCube operator+(const uword val) const;
  inline SizeCube operator-(const uword val) const;

  inline SizeCube operator*(const uword val) const;
  inline SizeCube operator/(const uword val) const;
  };



//! @}
