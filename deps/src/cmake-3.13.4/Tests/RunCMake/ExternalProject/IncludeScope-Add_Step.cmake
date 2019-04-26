function(IncludeScope_DefineProj)
  include(ExternalProject)
  ExternalProject_Add(MyProj
    SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}
    CONFIGURE_COMMAND ""
    BUILD_COMMAND     ""
    INSTALL_COMMAND   ""
  )
endfunction()

IncludeScope_DefineProj()

ExternalProject_Add_Step(MyProj extraStep COMMENT "Foo")
