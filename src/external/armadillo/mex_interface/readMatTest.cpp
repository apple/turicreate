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
// Version 0.5


#include "armaMex.hpp"


void
mexFunction(int nlhs, mxArray *plhs[], int nrhs, const mxArray *prhs[])
  {
  // Read the matrix from the file inData.mat
  mat fromFile = armaReadMatFromFile("inData.mat");

  fromFile.print();

  mat tmp(4,6);
  tmp.randu();

  // Write the matrix tmp as outData in the file outData.mat
  armaWriteMatToFile("outData.mat", tmp, "outData");
  }
