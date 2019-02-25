project(MyThirdPartyDependency)
cmake_minimum_required(VERSION 3.8)

include(CTest)

set_directory_properties(PROPERTIES LABELS "NotASubproject;MyThirdPartyDependency")
add_subdirectory(src)
