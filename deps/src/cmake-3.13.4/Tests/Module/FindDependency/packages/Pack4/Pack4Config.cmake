
include(CMakeFindDependencyMacro)

find_dependency(Pack5 3.1) # Actual version is 3.3. EXACT not propagated.
find_dependency(Pack6 5.5 EXACT)

add_library(Pack4::Lib INTERFACE IMPORTED)
set_property(TARGET Pack4::Lib PROPERTY INTERFACE_COMPILE_DEFINITIONS HAVE_PACK4)
set_property(TARGET Pack4::Lib PROPERTY INTERFACE_LINK_LIBRARIES Pack5::Lib Pack6::Lib)
