#ifdef __APPLE__
#  include <OpenCL/opencl.h>
#else
#  include <CL/cl.h>
#endif

int main()
{
  cl_uint platformIdCount;

  // We can't assert on the result because this may return an error if no ICD
  // is
  // found
  clGetPlatformIDs(0, NULL, &platformIdCount);

  return 0;
}
