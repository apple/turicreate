project(TuriExternalDependencies)

add_library(eigen INTERFACE)
target_compile_definitions(eigen INTERFACE EIGEN_MPL2_ONLY)
