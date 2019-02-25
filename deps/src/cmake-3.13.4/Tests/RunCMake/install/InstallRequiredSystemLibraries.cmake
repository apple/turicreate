enable_language(C)
set(CMAKE_INSTALL_MFC_LIBRARIES 1)
set(CMAKE_INSTALL_DEBUG_LIBRARIES 1)
set(CMAKE_INSTALL_UCRT_LIBRARIES 1)
set(CMAKE_INSTALL_OPENMP_LIBRARIES 1)
include(InstallRequiredSystemLibraries)

# FIXME: This test emits warnings because InstallRequiredSystemLibraries
# doesn't currently work properly. The warnings have been suppressed in
# InstallRequiredSystemLibraries-stderr.txt. This needs to be fixed.
