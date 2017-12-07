# Test moc include patterns

set(CMAKE_AUTOMOC_RELAXED_MODE FALSE)

include_directories("../mocInclude")

add_executable(mocIncludeStrict
  ../mocInclude/ObjA.cpp
  ../mocInclude/ObjB.cpp
  ../mocInclude/ObjC.cpp
  ../mocInclude/ObjD.cpp
  ../mocInclude/subA/SubObjA.cpp
  ../mocInclude/subB/SubObjB.cpp
  ../mocInclude/subC/SubObjC.cpp
  main.cpp
)
target_link_libraries(mocIncludeStrict ${QT_LIBRARIES})
set_target_properties(mocIncludeStrict PROPERTIES AUTOMOC ON)
