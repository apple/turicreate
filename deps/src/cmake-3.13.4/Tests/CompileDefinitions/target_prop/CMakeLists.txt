
project(target_prop)

add_executable(target_prop_executable ../compiletest.cpp)

set_target_properties(target_prop_executable PROPERTIES COMPILE_DEFINITIONS CMAKE_IS_FUN)

if (NOT NO_SPACES_IN_DEFINE_VALUES)
  set_property(TARGET target_prop_executable APPEND PROPERTY COMPILE_DEFINITIONS CMAKE_IS_REALLY="Very Fun" CMAKE_IS=Fun)
else()
  set_property(TARGET target_prop_executable APPEND PROPERTY COMPILE_DEFINITIONS CMAKE_IS=Fun)
endif()
set_property(TARGET target_prop_executable APPEND PROPERTY COMPILE_DEFINITIONS CMAKE_IS_FUN CMAKE_IS_="Fun")

set_property(TARGET target_prop_executable APPEND PROPERTY COMPILE_DEFINITIONS
  TEST_GENERATOR_EXPRESSIONS
    "$<1:CMAKE_IS_DECLARATIVE>"
    "$<0:GE_NOT_DEFINED>"
    "$<1:ARGUMENT;LIST>"
    PREFIX_$<JOIN:DEF1;DEF2,;PREFIX_>
    LETTER_LIST1=\"$<JOIN:A;B;C;D,,>\"
    LETTER_LIST2=\"$<JOIN:A;B;C;D,,,>\"
    LETTER_LIST3=\"$<JOIN:A;B;C;D,,->\"
    LETTER_LIST4=\"$<JOIN:A;B;C;D,-,->\"
    LETTER_LIST5=\"$<JOIN:A;B;C;D,-,>\"
    "$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,CXX>:LINK_CXX_DEFINE>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,C>:LINK_C_DEFINE>"
    "LINK_LANGUAGE_IS_$<TARGET_PROPERTY:LINKER_LANGUAGE>"
)

set_property(TARGET target_prop_executable APPEND PROPERTY COMPILE_DEFINITIONS
  BUILD_IS_DEBUG=$<CONFIG:Debug>
  BUILD_IS_NOT_DEBUG=$<NOT:$<CONFIG:Debug>>
  )

add_executable(target_prop_c_executable ../compiletest.c)

cmake_policy(SET CMP0043 NEW)
set_property(TARGET target_prop_c_executable APPEND PROPERTY COMPILE_DEFINITIONS_DEBUG DEBUG_MODE)

set_property(TARGET target_prop_c_executable APPEND PROPERTY COMPILE_DEFINITIONS
    "$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,CXX>:LINK_CXX_DEFINE>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,C>:LINK_C_DEFINE>"
    "LINK_LANGUAGE_IS_$<TARGET_PROPERTY:LINKER_LANGUAGE>"
    )

# Resulting link language will be CXX
add_executable(target_prop_mixed_executable ../compiletest_mixed_c.c ../compiletest_mixed_cxx.cpp)

set_property(TARGET target_prop_mixed_executable APPEND PROPERTY COMPILE_DEFINITIONS
    "$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,CXX>:LINK_CXX_DEFINE>"
    "$<$<STREQUAL:$<TARGET_PROPERTY:LINKER_LANGUAGE>,C>:LINK_C_DEFINE>"
    "LINK_LANGUAGE_IS_$<TARGET_PROPERTY:LINKER_LANGUAGE>"
    "C_EXECUTABLE_LINK_LANGUAGE_IS_$<TARGET_PROPERTY:target_prop_c_executable,LINKER_LANGUAGE>"
    )

add_library(tgt STATIC IMPORTED)
set_property(TARGET tgt APPEND PROPERTY COMPILE_DEFINITIONS TGT_DEF TGT_TYPE_$<TARGET_PROPERTY:TYPE>)
add_executable(usetgt usetgt.c)
target_compile_definitions(usetgt PRIVATE $<TARGET_PROPERTY:tgt,COMPILE_DEFINITIONS>)
