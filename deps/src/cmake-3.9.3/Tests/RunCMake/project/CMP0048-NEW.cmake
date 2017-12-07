macro(print_versions name)
  foreach(v "" _MAJOR _MINOR _PATCH _TWEAK)
    message(STATUS "PROJECT_VERSION${v}='${PROJECT_VERSION${v}}'")
    message(STATUS "${name}_VERSION${v}='${${name}_VERSION${v}}'")
  endforeach()
endmacro()

cmake_policy(SET CMP0048 NEW)

project(ProjectA VERSION 1.2.3.4 LANGUAGES NONE)
print_versions(ProjectA)

project(ProjectB VERSION 0.1.2 LANGUAGES NONE)
print_versions(ProjectB)

set(PROJECT_VERSION 1)
set(ProjectC_VERSION 1)
project(ProjectC NONE)
print_versions(ProjectC)
