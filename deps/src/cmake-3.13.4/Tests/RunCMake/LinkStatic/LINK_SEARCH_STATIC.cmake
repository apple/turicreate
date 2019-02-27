enable_language(C)

set(CMAKE_LINK_SEARCH_START_STATIC ON)
add_executable(LinkSearchStartStaticInit1 LinkStatic.c)
get_target_property(LSSS LinkSearchStartStaticInit1
  LINK_SEARCH_START_STATIC)
if(NOT LSSS)
  message(FATAL_ERROR "Failed to correctly initialize LINK_SEARCH_START_STATIC")
endif()
unset(CMAKE_LINK_SEARCH_START_STATIC)

add_executable(LinkSearchStartStaticSet1 LinkStatic.c)
set_target_properties(LinkSearchStartStaticSet1 PROPERTIES
  LINK_SEARCH_START_STATIC ON)
get_target_property(LSSS LinkSearchStartStaticSet1
  LINK_SEARCH_START_STATIC)
if(NOT LSSS)
  message(FATAL_ERROR "Failed to correctly set LINK_SEARCH_START_STATIC")
endif()

set(CMAKE_LINK_SEARCH_START_STATIC OFF)
add_executable(LinkSearchStartStaticInit2 LinkStatic.c)
get_target_property(LSSS LinkSearchStartStaticInit2
  LINK_SEARCH_START_STATIC)
if(LSSS)
  message(FATAL_ERROR "Failed to correctly initialize LINK_SEARCH_START_STATIC")
endif()
unset(CMAKE_LINK_SEARCH_START_STATIC)

add_executable(LinkSearchStartStaticSet2 LinkStatic.c)
set_target_properties(LinkSearchStartStaticSet2 PROPERTIES
  LINK_SEARCH_START_STATIC OFF)
get_target_property(LSSS LinkSearchStartStaticSet2
  LINK_SEARCH_START_STATIC)
if(LSSS)
  message(FATAL_ERROR "Failed to correctly set LINK_SEARCH_START_STATIC")
endif()

set(CMAKE_LINK_SEARCH_END_STATIC ON)
add_executable(LinkSearchEndStaticInit1 LinkStatic.c)
get_target_property(LSES LinkSearchEndStaticInit1
  LINK_SEARCH_END_STATIC)
if(NOT LSES)
  message(FATAL_ERROR "Failed to correctly initialize LINK_SEARCH_END_STATIC")
endif()
unset(CMAKE_LINK_SEARCH_END_STATIC)

add_executable(LinkSearchEndStaticSet1 LinkStatic.c)
set_target_properties(LinkSearchEndStaticSet1 PROPERTIES
  LINK_SEARCH_END_STATIC ON)
get_target_property(LSSS LinkSearchEndStaticSet1
  LINK_SEARCH_END_STATIC)
if(NOT LSSS)
  message(FATAL_ERROR "Failed to correctly set LINK_SEARCH_END_STATIC")
endif()

set(CMAKE_LINK_SEARCH_END_STATIC OFF)
add_executable(LinkSearchEndStaticInit2 LinkStatic.c)
get_target_property(LSES LinkSearchEndStaticInit2
  LINK_SEARCH_END_STATIC)
if(LSES)
  message(FATAL_ERROR "Failed to correctly initialize LINK_SEARCH_END_STATIC")
endif()
unset(CMAKE_LINK_SEARCH_END_STATIC)

add_executable(LinkSearchEndStaticSet2 LinkStatic.c)
set_target_properties(LinkSearchEndStaticSet2 PROPERTIES
  LINK_SEARCH_END_STATIC ON)
get_target_property(LSSS LinkSearchEndStaticSet2
  LINK_SEARCH_END_STATIC)
if(NOT LSSS)
  message(FATAL_ERROR "Failed to correctly set LINK_SEARCH_END_STATIC")
endif()
