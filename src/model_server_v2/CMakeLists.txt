make_library( model_server_v2
  SOURCES
    model_server.cpp
    model_base.cpp
  REQUIRES
    unity_shared)


make_executable(demo 
  SOURCES demo.cpp
  REQUIRES model_server_v2 unity_shared)






