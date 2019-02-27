
// Visual Studio allows only one set of flags for C and C++.
// In a target using C++ we pick the C++ flags even for C sources.
#ifdef TEST_LANG_DEFINES_FOR_VISUAL_STUDIO_OR_XCODE
#  include "cxx_only.h"

#  ifndef CXX_ONLY_DEFINE
#    error Expected CXX_ONLY_DEFINE
#  endif
#else
#  include "c_only.h"

#  ifndef C_ONLY_DEFINE
#    error Expected C_ONLY_DEFINE
#  endif
#endif

int consumer_c()
{
  return 0;
}
