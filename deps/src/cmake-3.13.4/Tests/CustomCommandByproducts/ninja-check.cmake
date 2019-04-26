file(READ build.ninja build_ninja)
if("${build_ninja}" MATCHES [====[
# Unknown Build Time Dependencies.
# Tell Ninja that they may appear as side effects of build rules
# otherwise ordered by order-only dependencies.

((build [^:]*: phony[^\n]*
)*)# ========]====])
  set(phony "${CMAKE_MATCH_1}")
  if(NOT phony)
    message(STATUS "build.ninja correctly does not have extra phony rules")
  else()
    string(REGEX REPLACE "\n+$" "" phony "${phony}")
    string(REGEX REPLACE "\n" "\n  " phony "  ${phony}")
    message(FATAL_ERROR "build.ninja incorrectly has extra phony rules:\n"
      "${phony}")
  endif()
else()
  message(FATAL_ERROR "build.ninja is incorrectly missing expected block")
endif()
