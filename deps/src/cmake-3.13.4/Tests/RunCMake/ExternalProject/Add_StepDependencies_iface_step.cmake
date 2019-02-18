include(ExternalProject)

ExternalProject_Add(MyProj
  DOWNLOAD_COMMAND ""
  CONFIGURE_COMMAND ""
  BUILD_COMMAND ""
  INSTALL_COMMAND ""
  )

add_library(MyProj-IFace INTERFACE)
ExternalProject_Add_StepDependencies(MyProj IFace dep)
