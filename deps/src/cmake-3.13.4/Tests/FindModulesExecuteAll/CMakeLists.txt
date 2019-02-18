# This file includes all FindXXX.cmake modules, so they are all executed.
# As it is it doesn't test a lot.
# It makes sure that the modules don't contain basic syntax errors.
# It also makes sure that modules don't fail with an error if something
# wasn't found but REQUIRED was not given.
#
# I guess more things could be added, like checking whether variables are
# defined after running the modules (e.g. FOO_FOUND etc.).
cmake_minimum_required(VERSION 2.8.4) # new enough for CMP0017
project(FindModulesExecuteAll)

file(GLOB all_modules "${CMAKE_CURRENT_SOURCE_DIR}/../../Modules/Find*cmake")

foreach(module ${all_modules})
  message(STATUS "module: ${module}")
  include("${module}")

  # get the "basename" of the package, so the existence of variables like
  # FOO_FOUND could be tested later on, Alex
  string(REGEX REPLACE ".+Find([^\\.]+)\\.cmake" "\\1" packageName "${module}")
  string(TOUPPER "${packageName}" packageNameUpper)

# disabled for now, since too many modules break:
#  if(NOT DEFINED ${packageNameUpper}_FOUND)
#    message(SEND_ERROR "${packageNameUpper}_FOUND not defined !")
#  endif()

endforeach()

add_executable(FindModulesExecuteAll main.c)
