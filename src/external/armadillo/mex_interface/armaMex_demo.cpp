// Copyright 2014 Conrad Sanderson (http://conradsanderson.id.au)
// Copyright 2014 National ICT Australia (NICTA)
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


// Demonstration of how to connect Armadillo with Matlab mex functions.
// Version 0.2


#include "armaMex.hpp"


void
mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
  {
  // Check the number of input arguments.
  if (nrhs != 2)
    mexErrMsgTxt("Incorrect number of input arguments.");

  // Check type of input.
  if ( (mxGetClassID(prhs[0]) != mxDOUBLE_CLASS) || (mxGetClassID(prhs[1]) != mxDOUBLE_CLASS) )
    mexErrMsgTxt("Input must me of type double.");

  // Check if input is real.
  if ( (mxIsComplex(prhs[0])) || (mxIsComplex(prhs[1])) )
    mexErrMsgTxt("Input must be real.");

  // Create matrices X and Y from the first and second argument.
  mat X = armaGetPr(prhs[0]);
  mat Y = armaGetPr(prhs[1]);

  // Our calculations require that matrices must be of the same size
  if ( size(X) != size(Y) )
    mexErrMsgTxt("Matrices should be of same size.");

  // Perform calculations
  mat A = X + Y;
  mat B = X % Y;  // % means element-wise multiplication in Armadillo

  // Create cube C with A and B as slices.
  cube C(A.n_rows, A.n_cols, 2);

  C.slice(0) = A;
  C.slice(1) = B;

  // Create the output argument plhs[0] to return cube C
  plhs[0] = armaCreateMxMatrix(C.n_rows, C.n_cols, C.n_slices);

  // Return the cube C as plhs[0] in Matlab/Octave
  armaSetCubePr(plhs[0], C);

  return;
  }
