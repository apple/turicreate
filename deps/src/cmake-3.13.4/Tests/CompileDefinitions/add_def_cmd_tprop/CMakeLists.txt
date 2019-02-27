add_definitions(-DCMAKE_IS_FUN -DCMAKE_IS=Fun)

add_executable(add_def_cmd_tprop_exe ../compiletest.cpp)

set_target_properties(add_def_cmd_tprop_exe PROPERTIES COMPILE_DEFINITIONS CMAKE_IS_="Fun")

if (NOT NO_SPACES_IN_DEFINE_VALUES)
  set_property(TARGET add_def_cmd_tprop_exe APPEND PROPERTY COMPILE_DEFINITIONS CMAKE_IS_REALLY="Very Fun")
endif()

add_definitions(-DCMAKE_IS_FUN)

set_property(TARGET add_def_cmd_tprop_exe APPEND PROPERTY COMPILE_DEFINITIONS CMAKE_IS=Fun CMAKE_IS_="Fun")

add_definitions(-DBUILD_IS_DEBUG=$<CONFIG:Debug>)
set_property(TARGET add_def_cmd_tprop_exe APPEND PROPERTY COMPILE_DEFINITIONS BUILD_IS_NOT_DEBUG=$<NOT:$<CONFIG:Debug>>)
