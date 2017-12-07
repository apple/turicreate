include(ExternalProject)

add_library(SomeInterface INTERFACE)
ExternalProject_Add_StepDependencies(SomeInterface step dep)
