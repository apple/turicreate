cmake_minimum_required(VERSION 3.10)
project(MocSkipSource)
include("../AutogenTest.cmake")

# Test for SKIP_AUTOMOC and SKIP_AUTOGEN on an AUTOMOC enabled target

# Generate header mocs manually
qtx_wrap_cpp(skipMocWrapMoc
  qItemA.hpp
  qItemB.hpp
  qItemC.hpp
  qItemD.hpp
)
set(skipMocSources
  skipMoc.cpp
  qItemA.cpp
  qItemB.cpp
  qItemC.cpp
  qItemD.cpp
)
# When cpp files are skipped, the hpp won't be processed either,
# unless they are mentioned in the sources - which they aren't.
set_property(SOURCE qItemA.cpp PROPERTY SKIP_AUTOMOC ON)
set_property(SOURCE qItemB.cpp PROPERTY SKIP_AUTOGEN ON)
# When hpp files are skipped, the cpp still get processed.
set_property(SOURCE qItemC.hpp PROPERTY SKIP_AUTOMOC ON)
set_property(SOURCE qItemD.hpp PROPERTY SKIP_AUTOGEN ON)
# AUTOMOC enabled only
add_executable(skipMocA ${skipMocSources} ${skipMocWrapMoc})
set_property(TARGET skipMocA PROPERTY AUTOMOC ON)
target_link_libraries(skipMocA ${QT_LIBRARIES})
# AUTOMOC and AUTOUIC enabled
add_executable(skipMocB ${skipMocSources} ${skipMocWrapMoc})
set_property(TARGET skipMocB PROPERTY AUTOMOC ON)
set_property(TARGET skipMocB PROPERTY AUTOUIC ON)
target_link_libraries(skipMocB ${QT_LIBRARIES})
