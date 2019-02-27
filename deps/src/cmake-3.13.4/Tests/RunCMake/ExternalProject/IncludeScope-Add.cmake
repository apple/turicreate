function(IncludeScope_IncludeOnly)
  include(ExternalProject)
endfunction()

IncludeScope_IncludeOnly()

ExternalProject_Add(MyProj
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND     ""
    INSTALL_COMMAND   ""
)
