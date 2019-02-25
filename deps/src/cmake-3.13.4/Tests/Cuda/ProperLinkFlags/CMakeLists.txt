
cmake_minimum_required(VERSION 3.7)
project (ProperLinkFlags CUDA CXX)

#Goal for this example:
#Verify that when we have CXX and CUDA enabled and we link an executable that
#has CUDA and CXX we use the CUDA link flags when doing the device link
#step

#Specify a set of valid CUDA flags and an invalid set of CXX flags ( for CUDA )
#to make sure we don't use the CXX flags when linking CUDA executables
string(APPEND CMAKE_CUDA_FLAGS " -arch=sm_35 --use_fast_math")
set(CMAKE_CXX_FLAGS "-Wall")

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CUDA_STANDARD 11)
add_executable(ProperLinkFlags file1.cu main.cxx)

set_target_properties( ProperLinkFlags
                       PROPERTIES CUDA_SEPARABLE_COMPILATION ON)
