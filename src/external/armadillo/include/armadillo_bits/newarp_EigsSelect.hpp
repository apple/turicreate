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


namespace newarp
{


//! The enumeration of selection rules of desired eigenvalues.
struct EigsSelect
  {
  enum SELECT_EIGENVALUE
    {
    LARGEST_MAGN = 0,  //!< Select eigenvalues with largest magnitude.
                       //!< Magnitude means the absolute value for real numbers and norm for complex numbers.
                       //!< Applies to both symmetric and general eigen solvers.

    LARGEST_REAL,      //!< Select eigenvalues with largest real part. Only for general eigen solvers.

    LARGEST_IMAG,      //!< Select eigenvalues with largest imaginary part (in magnitude). Only for general eigen solvers.

    LARGEST_ALGE,      //!< Select eigenvalues with largest algebraic value, considering any negative sign. Only for symmetric eigen solvers.

    SMALLEST_MAGN,     //!< Select eigenvalues with smallest magnitude. Applies to both symmetric and general eigen solvers.

    SMALLEST_REAL,     //!< Select eigenvalues with smallest real part. Only for general eigen solvers.

    SMALLEST_IMAG,     //!< Select eigenvalues with smallest imaginary part (in magnitude). Only for general eigen solvers.

    SMALLEST_ALGE,     //!< Select eigenvalues with smallest algebraic value. Only for symmetric eigen solvers.

    BOTH_ENDS          //!< Select eigenvalues half from each end of the spectrum.
                       //!< When `nev` is odd, compute more from the high end. Only for symmetric eigen solvers.
    };
  };


}  // namespace newarp
