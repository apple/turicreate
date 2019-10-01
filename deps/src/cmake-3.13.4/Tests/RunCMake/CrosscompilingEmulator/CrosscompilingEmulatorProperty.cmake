# This tests setting the CROSSCOMPILING_EMULATOR target property from the
# CMAKE_CROSSCOMPILING_EMULATOR variable.

# -DCMAKE_CROSSCOMPILING_EMULATOR=/path/to/pseudo_emulator is passed to this
# test
add_executable(target_with_emulator simple_src_exiterror.cxx)
get_property(emulator TARGET target_with_emulator
             PROPERTY CROSSCOMPILING_EMULATOR)
if(NOT "${emulator}" MATCHES "pseudo_emulator")
  message(SEND_ERROR "Default CROSSCOMPILING_EMULATOR property not set")
endif()

set_property(TARGET target_with_emulator
             PROPERTY CROSSCOMPILING_EMULATOR "another_emulator")
get_property(emulator TARGET target_with_emulator
             PROPERTY CROSSCOMPILING_EMULATOR)
if(NOT "${emulator}" MATCHES "another_emulator")
  message(SEND_ERROR
    "set_property/get_property CROSSCOMPILING_EMULATOR is not consistent")
endif()

unset(CMAKE_CROSSCOMPILING_EMULATOR CACHE)
add_executable(target_without_emulator simple_src_exiterror.cxx)
get_property(emulator TARGET target_without_emulator
             PROPERTY CROSSCOMPILING_EMULATOR)
if(NOT "${emulator}" STREQUAL "")
  message(SEND_ERROR "Default CROSSCOMPILING_EMULATOR property not set to null")
endif()
