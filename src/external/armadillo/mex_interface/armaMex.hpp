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


// Connector for Mex files to use Armadillo for calculation
// Version 0.5


#include <numerics/armadillo.hpp>
#include <mex.h>
#include <mat.h>
#include <cstring>

using namespace std;
using namespace arma;


// Get scalar value from Matlab/Octave
template<class Type>
inline
Type
armaGetScalar(const mxArray *matlabScalar)
  {
  if(mxGetData(matlabScalar) != NULL)
    {
    return (Type)mxGetScalar(matlabScalar);
    }
  else
    {
    mexErrMsgTxt("No data available.");
    return 0;
    }
  }


// To keep with Matlab/Octave mex functions since functions for double are usually defined in conjunction with the general functions.
inline
double
armaGetDouble(const mxArray *matlabScalar)
  {
  return armaGetScalar<double>(matlabScalar);
  }


// Get non-double real matrix from Matlab/Octave. Type should be case according to input.
// Use mxGetClassID inside main program to test for type.
template<class Type>
inline
Mat<Type>
armaGetData(const mxArray *matlabMatrix, bool copy_aux_mem = false, bool strict = true)
  {
  if(mxGetData(matlabMatrix) != NULL)
    {
    const mwSize n_dim = mxGetNumberOfDimensions(matlabMatrix);

    if(n_dim == 2)
      {
      return Mat<Type>((Type *)mxGetData(matlabMatrix), mxGetM(matlabMatrix), mxGetN(matlabMatrix), copy_aux_mem, strict);
      }
    else
      {
      mexErrMsgTxt("Number of dimensions must be 2.");
      return Mat<Type>();
      }
    }
  else
    {
    mexErrMsgTxt("No data available.");
    return Mat<Type>();
    }
  }


// Get double real matrix from Matlab/Octave.
inline
Mat<double>
armaGetPr(const mxArray *matlabMatrix, bool copy_aux_mem = false, bool strict = true)
  {
  if(mxGetData(matlabMatrix) != NULL)
    {
    const mwSize n_dim = mxGetNumberOfDimensions(matlabMatrix);

    if(n_dim == 2)
      {
      return Mat<double>(mxGetPr(matlabMatrix), mxGetM(matlabMatrix), mxGetN(matlabMatrix), copy_aux_mem, strict);
      }
    else
      {
      mexErrMsgTxt("Number of dimensions must be 2.");
      return Mat<double>();
      }
    }
  else
    {
    mexErrMsgTxt("No data available.");
    return Mat<double>();
    }
  }


// Get non-double imaginary matrix from Matlab/Octave. Type should be case according to input.
// Use mxGetClassID inside main program to test for type.
template<class Type>
inline
Mat<Type>
armaGetImagData(const mxArray *matlabMatrix, bool copy_aux_mem = false, bool strict = true)
  {
  if(mxGetImagData(matlabMatrix) != NULL)
    {
    const mwSize n_dim = mxGetNumberOfDimensions(matlabMatrix);

    if(n_dim == 2)
      {
      return Mat<Type>((Type *)mxGetImagData(matlabMatrix), mxGetM(matlabMatrix), mxGetN(matlabMatrix), copy_aux_mem, strict);
      }
    else
      {
      mexErrMsgTxt("Number of dimensions must be 2.");
      return Mat<Type>();
      }
    }
  else
    {
    mexErrMsgTxt("No data available.");
    return Mat<Type>();
    }
  }


// Get double imaginary matrix from Matlab/Octave.
inline
Mat<double>
armaGetPi(const mxArray *matlabMatrix, bool copy_aux_mem = false, bool strict = true)
  {
  if(mxGetImagData(matlabMatrix) != NULL)
    {
    const mwSize n_dim = mxGetNumberOfDimensions(matlabMatrix);

    if(n_dim == 2)
      {
      return Mat<double>(mxGetPi(matlabMatrix), mxGetM(matlabMatrix), mxGetN(matlabMatrix), copy_aux_mem, strict);
      }
    else
      {
      mexErrMsgTxt("Number of dimensions must be 2.");
      return Mat<double>();
      }
    }
  else
    {
    mexErrMsgTxt("No data available.");
    return Mat<double>();
    }
  }


// Get complex matrix from Matlab/Octave
inline
cx_mat
armaGetCx(const mxArray *matlabMatrix, bool copy_aux_mem = false, bool strict = true)
  {
  if( (mxGetPr(matlabMatrix) != NULL) && (mxGetPi(matlabMatrix) != NULL) )
    {
    return cx_mat(armaGetPr(matlabMatrix, copy_aux_mem, strict), armaGetPi(matlabMatrix, copy_aux_mem, strict));
    }
  else if( (mxGetPr(matlabMatrix) != NULL) && (mxGetPi(matlabMatrix) == NULL) )
    {
    return cx_mat(armaGetPr(matlabMatrix, copy_aux_mem, strict), zeros(mxGetM(matlabMatrix),mxGetN(matlabMatrix)));
    }
  else if( (mxGetPr(matlabMatrix) == NULL) && (mxGetPi(matlabMatrix) != NULL) )
    {
    return cx_mat(zeros(mxGetM(matlabMatrix), mxGetN(matlabMatrix)), armaGetPi(matlabMatrix, copy_aux_mem, strict));
    }
  else
    {
    mexErrMsgTxt("No data available.");
    return cx_mat();
    }
  }


// Return non-double real valued matrix to Matlab/Octave
template<class Type>
inline
void
armaSetData(mxArray *matlabMatrix, const Mat<Type>& armaMatrix)
  {
        Type *dst_pointer = (Type*)mxGetData(matlabMatrix);
  const Type *src_pointer = (Type*)armaMatrix.memptr();

  std::memcpy(dst_pointer, src_pointer, sizeof(Type)*armaMatrix.n_elem);
  }


// Return double real valued matrix to Matlab/Octave
inline
void
armaSetPr(mxArray *matlabMatrix, const Mat<double>& armaMatrix)
  {
        double *dst_pointer = mxGetPr(matlabMatrix);
  const double *src_pointer = armaMatrix.memptr();

  std::memcpy(dst_pointer, src_pointer, sizeof(double)*armaMatrix.n_elem);
  }


// Return imaginary valued matrix to Matlab/Octave.
template<class Type>
inline
void
armaSetImagData(mxArray *matlabMatrix, const Mat<Type>& armaMatrix)
  {
        Type *dst_pointer = (Type*)mxGetImagData(matlabMatrix);
  const Type *src_pointer = (Type*)armaMatrix.memptr();

  std::memcpy(dst_pointer, src_pointer, sizeof(Type)*armaMatrix.n_elem);
  }


// Return double complex valued matrix to Matlab/Octave
inline
void
armaSetPi(mxArray *matlabMatrix, const Mat<double>& armaMatrix)
  {
        double *dst_pointer = mxGetPi(matlabMatrix);
  const double *src_pointer = armaMatrix.memptr();

  std::memcpy(dst_pointer, src_pointer, sizeof(double)*armaMatrix.n_elem);
  }


// Return complex matrix to Matlab/Octave. Requires Matlab/Octave matrix to be mxCOMPLEX
inline
void
armaSetCx(mxArray *matlabMatrix, const cx_mat& armaMatrix)
  {
  armaSetPr(matlabMatrix, real(armaMatrix));
  armaSetPi(matlabMatrix, imag(armaMatrix));
  }


// Cube functions

// Get non-double real cube from Matlab/Octave. Type should be case according to input.
// Use mxGetClassID inside main program to test for type.
template<class Type>
inline
Cube<Type>
armaGetCubeData(const mxArray *matlabMatrix, bool copy_aux_mem = false, bool strict = true)
  {
  if(mxGetData(matlabMatrix) != NULL)
    {
    const mwSize n_dim = mxGetNumberOfDimensions(matlabMatrix);

    if(n_dim == 3)
      {
      const mwSize *dims = mxGetDimensions(matlabMatrix);
      return Cube<Type>((Type *)mxGetData(matlabMatrix), dims[0], dims[1], dims[2], copy_aux_mem, strict);
      }
    else
      {
      mexErrMsgTxt("Number of dimensions must be 3.");
      return Cube<Type>();
      }
    }
  else
    {
    mexErrMsgTxt("No data available.");
    return Cube<Type>();
    }
  }


// Get double cube from Matlab/Octave.
inline
Cube<double>
armaGetCubePr(const mxArray *matlabMatrix, bool copy_aux_mem = false, bool strict = true)
  {
  if(mxGetData(matlabMatrix) != NULL)
    {
    const mwSize n_dim = mxGetNumberOfDimensions(matlabMatrix);

    if(n_dim == 3)
      {
      const mwSize *dims = mxGetDimensions(matlabMatrix);
      return Cube<double>(mxGetPr(matlabMatrix), dims[0], dims[1], dims[2], copy_aux_mem, strict);
      }
    else
      {
      mexErrMsgTxt("Number of dimensions must be 3.");
      return Cube<double>();
      }
    }
  else
    {
    mexErrMsgTxt("No data available.");
    return Cube<double>();
    }
  }


// Get non-double imaginary cube from Matlab/Octave. Type should be case according to input.
// Use mxGetClassID inside main program to test for type.
template<class Type>
inline
Cube<Type>
armaGetCubeImagData(const mxArray *matlabMatrix, bool copy_aux_mem = false, bool strict = true)
  {
  if(mxGetImagData(matlabMatrix) != NULL)
    {
    const mwSize n_dim = mxGetNumberOfDimensions(matlabMatrix);

    if(n_dim == 3)
      {
      const mwSize *dims = mxGetDimensions(matlabMatrix);
      return Cube<Type>((Type *)mxGetImagData(matlabMatrix), dims[0], dims[1], dims[2], copy_aux_mem, strict);
      }
    else
      {
      mexErrMsgTxt("Number of dimensions must be 3.");
      return Cube<Type>();
      }
    }
  else
    {
    mexErrMsgTxt("No data available.");
    return Cube<Type>();
    }
  }


// Get double cube from Matlab/Octave.
inline
Cube<double>
armaGetCubePi(const mxArray *matlabMatrix, bool copy_aux_mem = false, bool strict = true)
  {
  if(mxGetImagData(matlabMatrix) != NULL)
    {
    const mwSize n_dim = mxGetNumberOfDimensions(matlabMatrix);

    if(n_dim == 3)
      {
      const mwSize *dims = mxGetDimensions(matlabMatrix);
      return Cube<double>(mxGetPi(matlabMatrix), dims[0], dims[1], dims[2], copy_aux_mem, strict);
      }
    else
      {
      mexErrMsgTxt("Number of dimensions must be 3.");
      return Cube<double>();
      }
    }
  else
    {
    mexErrMsgTxt("No data available.");
    return Cube<double>();
    }
  }


// Get complex cube from Matlab/Octave
inline
cx_cube
armaGetCubeCx(const mxArray *matlabMatrix, bool copy_aux_mem = false, bool strict = true)
  {
  if( (mxGetPr(matlabMatrix) != NULL) && (mxGetPi(matlabMatrix) != NULL) )
    {
    return cx_cube(armaGetCubePr(matlabMatrix, copy_aux_mem, strict), armaGetCubePi(matlabMatrix, copy_aux_mem, strict));
    }
  else if( (mxGetPr(matlabMatrix) != NULL) && (mxGetPi(matlabMatrix) == NULL) )
    {
    const mwSize *dims = mxGetDimensions(matlabMatrix);
    return cx_cube(armaGetCubePr(matlabMatrix, copy_aux_mem, strict), zeros(dims[0], dims[1], dims[2]));
    }
  else if( (mxGetPr(matlabMatrix) == NULL) && (mxGetPi(matlabMatrix) != NULL) )
    {
    const mwSize *dims = mxGetDimensions(matlabMatrix);
    return cx_cube(zeros(dims[0], dims[1], dims[2]), armaGetCubePi(matlabMatrix, copy_aux_mem, strict));
    }
  else
    {
    mexErrMsgTxt("No data available.");
    return cx_cube();
    }
  }

// return real valued cube to Matlab/Octave
template<class Type>
inline
void
armaSetCubeData(mxArray *matlabMatrix, const Cube<Type>& armaCube)
  {
        Type *dst_pointer = (Type*)mxGetData(matlabMatrix);
  const Type *src_pointer = (Type*)armaCube.memptr();

  std::memcpy(dst_pointer, src_pointer, sizeof(Type)*armaCube.n_elem);
  }


// Return double real valued cube to Matlab/Octave
inline
void
armaSetCubePr(mxArray *matlabMatrix, const Cube<double>& armaCube)
  {
        double *dst_pointer = mxGetPr(matlabMatrix);
  const double *src_pointer = armaCube.memptr();

  std::memcpy(dst_pointer, src_pointer, sizeof(double)*armaCube.n_elem);
  }


// Return imaginary valued cube to Matlab/Octave.
template<class Type>
inline
void
armaSetImagCubeData(mxArray *matlabMatrix, const Cube<Type>& armaCube)
  {
        Type *dst_pointer = (Type*)mxGetImagData(matlabMatrix);
  const Type *src_pointer = (Type*)armaCube.memptr();

  std::memcpy(dst_pointer, src_pointer, sizeof(Type)*armaCube.n_elem);
  }


// Return double imaginary valued matrix to Matlab/Octave
inline
void
armaSetCubePi(mxArray *matlabMatrix, const Cube<double>& armaCube)
  {
        double *dst_pointer = mxGetPi(matlabMatrix);
  const double *src_pointer = armaCube.memptr();

  std::memcpy(dst_pointer, src_pointer, sizeof(double)*armaCube.n_elem);
  }


// Return double complex cube to Matlab/Octave.
inline
void
armaSetCubeCx(mxArray *matlabMatrix, const cx_cube& armaCube)
  {
  armaSetCubePr(matlabMatrix, real(armaCube));
  armaSetCubePi(matlabMatrix, imag(armaCube));
  }


// Sparse matrices


// Get sparse matrix from Matlab/Octave.
template<class Type>
inline
SpMat<Type>
armaGetSparseData(const mxArray *matlabMatrix, bool sort_locations = false)
  {
  if(!mxIsSparse(matlabMatrix))
    {
    mexErrMsgTxt("Matrix is not sparse.");
    return SpMat<Type>();
    }
  else
    {
    Type *pr = (Type *)mxGetData(matlabMatrix);

    if(pr == NULL)
      {
      mexErrMsgTxt("No data available.");
      return SpMat<Type>();
      }

    mwIndex *jc = mxGetJc(matlabMatrix);
    mwIndex *ir = mxGetIr(matlabMatrix);

    mwSize  m = mxGetM(matlabMatrix);
    mwSize  n = mxGetN(matlabMatrix);

    mwSize  non_zero = mxGetNzmax(matlabMatrix);

    umat locations = zeros<umat>(2,non_zero);
    Col<Type> values = zeros< Col<Type> >(non_zero);
    mwSize  row = 0;

    for(mwSize col = 0; col < n ; col++)
      {

      mwIndex starting_row_index = jc[col];
      mwIndex stopping_row_index = jc[col+1];

      if (starting_row_index == stopping_row_index)
        {
        // End of matrix when jc[col] == jc[col+1]
        continue;
        }
      else
        {
        for (mwIndex current_row_index = starting_row_index; current_row_index < stopping_row_index; current_row_index++)
          {
          values[row]=pr[row];
          locations.at(0,row)=ir[current_row_index];
          locations.at(1,row)=col;
          row++;
          }
        }
      }

    return SpMat<Type>(locations, values, m, n, sort_locations);
    }
  }


// Get double valued sparse matrix from Matlab/Octave.
inline
SpMat<double>
armaGetSparseMatrix(const mxArray *matlabMatrix, bool sort_locations = false)
  {
  if(!mxIsSparse(matlabMatrix))
    {
    mexErrMsgTxt("Matrix is not sparse.");
    return SpMat<double>();
    }
  else
    {
    double  *pr = mxGetPr(matlabMatrix);

    if(pr == NULL)
      {
      mexErrMsgTxt("No data available.");
      return SpMat<double>();
      }

    mwIndex *jc = mxGetJc(matlabMatrix);
    mwIndex *ir = mxGetIr(matlabMatrix);

    mwSize  m = mxGetM(matlabMatrix);
    mwSize  n = mxGetN(matlabMatrix);

    mwSize  non_zero = mxGetNzmax(matlabMatrix);

    umat locations = zeros<umat>(2,non_zero);
    Col<double> values = zeros< Col<double> >(non_zero);

    mwSize  row = 0;

    for(mwSize col = 0; col < n ; col++)
      {

      mwIndex starting_row_index = jc[col];
      mwIndex stopping_row_index = jc[col+1];

      if (starting_row_index == stopping_row_index)
        {
        // End of matrix when jc[col] == jc[col+1]
        continue;
        }
      else
        {
        for (mwIndex current_row_index = starting_row_index; current_row_index < stopping_row_index ; current_row_index++)
          {
          values[row]=pr[row];
          locations.at(0,row)=ir[current_row_index];
          locations.at(1,row)=col;
          row++;
          }
        }

      }
    return SpMat<double>(locations, values, m, n, sort_locations);
    }
  }


// Get imaginary sparse matrix from Matlab/Octave.
template<class Type>
inline
SpMat<Type>
armaGetSparseImagData(const mxArray *matlabMatrix, bool sort_locations = false)
  {
  if(!mxIsSparse(matlabMatrix))
    {
    mexErrMsgTxt("Matrix is not sparse.");
    return SpMat<Type>();
    }
  else
    {
    Type *pi = (Type *)mxGetImagData(matlabMatrix);

    if(pi == NULL)
      {
      mexErrMsgTxt("No data available.");
      return SpMat<Type>();
      }

    mwIndex *jc = mxGetJc(matlabMatrix);
    mwIndex *ir = mxGetIr(matlabMatrix);

    mwSize  m = mxGetM(matlabMatrix);
    mwSize  n = mxGetN(matlabMatrix);

    mwSize  non_zero = mxGetNzmax(matlabMatrix);

    umat locations = zeros<umat>(2,non_zero);
    Col<Type> values = zeros< Col<Type> >(non_zero);
    mwSize row = 0;

    for(mwSize col = 0; col < n ; col++)
      {
      mwIndex starting_row_index = jc[col];
      mwIndex stopping_row_index = jc[col+1];

      if (starting_row_index == stopping_row_index)
        {
        // End of matrix when jc[col] == jc[col+1]
        continue;
        }
      else
        {
        for (mwIndex current_row_index = starting_row_index; current_row_index < stopping_row_index; current_row_index++)
          {
          values[row]=pi[row];
          locations.at(0,row)=ir[current_row_index];
          locations.at(1,row)=col;
          row++;
          }
        }
      }

    return SpMat<Type>(locations, values, m, n, sort_locations);
    }
  }


// Get imaginary double valued sparse matrix from Matlab/Octave.
inline
SpMat<double>
armaGetSparseImagMatrix(const mxArray *matlabMatrix, bool sort_locations = false)
  {
  if(!mxIsSparse(matlabMatrix))
    {
    mexErrMsgTxt("Matrix is not sparse.");
    return SpMat<double>();
    }
  else
    {
    double  *pi = mxGetPi(matlabMatrix);

    if(pi == NULL)
      {
      mexErrMsgTxt("No data available.");
      return SpMat<double>();
      }

    mwIndex *jc = mxGetJc(matlabMatrix);
    mwIndex *ir = mxGetIr(matlabMatrix);

    mwSize  m = mxGetM(matlabMatrix);
    mwSize  n = mxGetN(matlabMatrix);

    mwSize  non_zero = mxGetNzmax(matlabMatrix);

    umat locations = zeros<umat>(2,non_zero);
    Col<double> values = zeros< Col<double> >(non_zero);

    mwSize row = 0;

    for(mwSize col = 0; col < n ; col++)
      {
      mwIndex starting_row_index = jc[col];
      mwIndex stopping_row_index = jc[col+1];

      if (starting_row_index == stopping_row_index)
        {
        // End of matrix when jc[col] == jc[col+1]
        continue;
        }
      else
        {
        for (mwIndex current_row_index = starting_row_index; current_row_index < stopping_row_index; current_row_index++)
          {
          values[row]=pi[row];
          locations.at(0,row)=ir[current_row_index];
          locations.at(1,row)=col;
          row++;
          }
        }
      }

    return SpMat<double>(locations, values, m, n, sort_locations);
    }
  }


// Return sparse matrix to matlab
inline
void
armaSetSparsePr(mxArray *matlabMatrix, const SpMat<double>& armaMatrix)
  {
  double  *sr  = mxGetPr(matlabMatrix);
  mwIndex *irs = mxGetIr(matlabMatrix);
  mwIndex *jcs = mxGetJc(matlabMatrix);

  mwSize n_nonzero = armaMatrix.n_nonzero;
  mwSize n_cols    = armaMatrix.n_cols;

  for (mwIndex j = 0; j < n_nonzero; j++)
    {
    sr[j]  = armaMatrix.values[j];
    irs[j] = armaMatrix.row_indices[j];
    }
  for (mwIndex j = 0; j <= n_cols; j++)
    {
    jcs[j] = armaMatrix.col_ptrs[j];
    }
  }


// Return sparse matrix to matlab as imaginary part
inline
void
armaSetSparsePi(mxArray *matlabMatrix, const SpMat<double>& armaMatrix)
  {
  double  *si  = mxGetPi(matlabMatrix);
  mwIndex *irs = mxGetIr(matlabMatrix);
  mwIndex *jcs = mxGetJc(matlabMatrix);

  mwSize n_nonzero = armaMatrix.n_nonzero;
  mwSize n_cols    = armaMatrix.n_cols;

  for (mwIndex j = 0; j < n_nonzero; j++)
    {
    si[j]  = armaMatrix.values[j];
    irs[j] = armaMatrix.row_indices[j];
    }
  for (mwIndex j = 0; j <= n_cols; j++)
    {
    jcs[j] = armaMatrix.col_ptrs[j];
    }
  }


// Create matlab side matrices


// Create 2-D Matlab/Octave matrix
inline
mxArray*
armaCreateMxMatrix(const mwSize n_rows, const mwSize n_cols, const mxClassID mx_type = mxDOUBLE_CLASS, const mxComplexity mx_complexity = mxREAL)
  {
  mxArray *temp = mxCreateNumericMatrix(n_rows, n_cols, mx_type, mx_complexity);

  if(temp == NULL)
    {
    mexErrMsgTxt("Could not create array.");
    return NULL;
    }
  else
    {
    return temp;
    }
  }


// Create 3-D Matlab/Octave matrix (cube)
inline
mxArray*
armaCreateMxMatrix(const mwSize n_rows, const mwSize n_cols, const mwSize n_slices, const mxClassID mx_type = mxDOUBLE_CLASS, const mxComplexity mx_complexity = mxREAL)
  {
  mwSize dims[3] = { n_rows, n_cols, n_slices };

  const mwSize n_dim = 3;

  mxArray *temp = mxCreateNumericArray(n_dim, dims, mx_type, mx_complexity);

  if(temp == NULL)
    {
    mexErrMsgTxt("Could not create array.");
    return NULL;
    }
  else
    {
    return temp;
    }
  }


inline
mxArray*
armaCreateMxSparseMatrix(const mwSize n_rows,const mwSize n_cols,const mwSize n_nonzero,const mxComplexity mx_complexity = mxREAL)
  {
  mxArray *temp = mxCreateSparse(n_rows, n_cols, n_nonzero, mx_complexity);

  if(temp == NULL)
    {
    mexErrMsgTxt("Could not create array.");
    return NULL;
    }
  else
    {
    return temp;
    }
  }


//Functions to write MAT files
inline
int
armaWriteMatToFile(const char *filename, mat &armaMatrix, const char *name)
  {
  MATFile *file;
  file = matOpen(filename,"wz");

  int result;

  if(file == NULL)
    {
    mexErrMsgTxt("Could not create MAT file.");
    return 0;
    }
  else
    {
    mxArray *temp = mxCreateDoubleMatrix(armaMatrix.n_rows, armaMatrix.n_cols, mxREAL);
    armaSetPr(temp, armaMatrix);
    result = matPutVariable(file, name, temp);
    mxDestroyArray(temp); //Cleanup after writing MAT file
    }

  matClose(file);

  return result;
  }


inline
int
armaWriteCxMatToFile(const char *filename, cx_mat &armaMatrix, const char *name)
  {
  MATFile *file;
  file = matOpen(filename,"wz");

  int result;

  if(file == NULL)
    {
    mexErrMsgTxt("Could not create MAT file.");
    return 0;
    }
  else
    {
    mxArray *temp = mxCreateDoubleMatrix(armaMatrix.n_rows, armaMatrix.n_cols, mxCOMPLEX);
    armaSetCx(temp, armaMatrix);
    result = matPutVariable(file, name, temp);
    mxDestroyArray(temp); //Cleanup after writing MAT file
    }

  matClose(file);

  return result;
  }


inline
int
armaWriteCubeToFile(const char *filename, cube &armaCube, const char *name)
  {
  MATFile *file;
  file = matOpen(filename,"wz");

  int result;

  if(file == NULL)
    {
    mexErrMsgTxt("Could not create MAT file.");
    return 0;
    }
  else
    {
    mxArray *temp = armaCreateMxMatrix(armaCube.n_rows, armaCube.n_cols, armaCube.n_slices, mxDOUBLE_CLASS, mxREAL);
    armaSetCubePr(temp, armaCube);
    result = matPutVariable(file, name, temp);
    mxDestroyArray(temp); //Cleanup after writing MAT file
    }

  matClose(file);

  return result;
  }


inline
int
armaWriteCxCubeToFile(const char *filename, cx_cube &armaCube, const char *name)
  {
  MATFile *file;
  file = matOpen(filename,"wz");

  int result;

  if(file == NULL)
    {
    mexErrMsgTxt("Could not create MAT file.");
    return 0;
    }
  else
    {
    mxArray *temp = armaCreateMxMatrix(armaCube.n_rows, armaCube.n_cols, armaCube.n_slices, mxDOUBLE_CLASS, mxCOMPLEX);
    armaSetCubeCx(temp, armaCube);
    result = matPutVariable(file, name, temp);
    mxDestroyArray(temp); //Cleanup after writing MAT file
    }

  matClose(file);

  return result;
  }


//Functions to read and write matrices and cubes in MAT file format
inline
mat
armaReadMatFromFile(const char *filename)
  {
  MATFile *file;
  file = matOpen(filename,"r");

  char buffer[1024];
  const char *name;
  name = buffer;

  if(file == NULL)
    {
    mexErrMsgTxt("Could not open MAT file.");
    return mat();
    }
  else
    {
    mat tmp = armaGetPr(matGetNextVariable(file,&name));
    matClose(file);

    return tmp;
    }
  }


inline
cx_mat
armaReadCxMatFromFile(const char *filename)
  {
  MATFile *file;
  file = matOpen(filename,"r");

  char buffer[1024];
  const char *name;
  name = buffer;

  if(file == NULL)
    {
    mexErrMsgTxt("Could not open MAT file.");
    return cx_mat();
    }
  else
    {
    cx_mat tmp = armaGetCx(matGetNextVariable(file, &name));
    matClose(file);

    return tmp;
    }
  }


inline
cube
armaReadCubeFromFile(const char *filename)
  {
  MATFile *file;
  file = matOpen(filename,"r");

  char buffer[1024];
  const char *name;
  name = buffer;

  if(file == NULL)
    {
    mexErrMsgTxt("Could not open MAT file.");
    return cube();
    }
  else
    {
    cube tmp = armaGetCubePr(matGetNextVariable(file,&name));
    matClose(file);

    return tmp;
    }
  }


inline
cx_cube
armaReadCxCubeFromFile(const char *filename)
  {
  MATFile *file;
  file = matOpen(filename,"r");

  char buffer[1024];
  const char *name;
  name = buffer;

  if(file == NULL)
    {
    mexErrMsgTxt("Could not open MAT file.");
    return cx_cube();
    }
  else
    {
    cx_cube tmp = armaGetCubeCx(matGetNextVariable(file,&name));
    matClose(file);

    return tmp;
    }
  }
