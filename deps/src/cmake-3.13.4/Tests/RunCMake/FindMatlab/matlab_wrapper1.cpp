
// simple workaround to some compiler specific problems
// see
// http://stackoverflow.com/questions/22367516/mex-compile-error-unknown-type-name-char16-t/23281916#23281916
#include <algorithm>

#include "mex.h"

// this test should return a matrix of 10 x 10 and should check some of the
// arguments

void mexFunction(const int nlhs, mxArray* plhs[], const int nrhs,
                 const mxArray* prhs[])
{
  if (nrhs != 1) {
    mexErrMsgTxt("Incorrect arguments");
  }

  size_t dim1 = mxGetM(prhs[0]);
  size_t dim2 = mxGetN(prhs[0]);

  if (dim1 == 1 || dim2 == 1) {
    mexErrMsgIdAndTxt("cmake_matlab:configuration", "Incorrect arguments");
  }

  plhs[0] = mxCreateNumericMatrix(dim1, dim2, mxGetClassID(prhs[0]), mxREAL);
}
