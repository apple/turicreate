
project(TuriExternalDependencies)


add_library(sparsehash INTERFACE )
target_compile_definitions(sparsehash INTERFACE HAS_SPARSEHASH google=__tc_google)
set(HAS_SPARSEHASH TRUE CACHE BOOL "")

