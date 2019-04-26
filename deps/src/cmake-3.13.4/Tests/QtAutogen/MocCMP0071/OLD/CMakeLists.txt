cmake_minimum_required(VERSION 3.10)
cmake_policy(SET CMP0071 OLD)

# *Generate* files
set(CSD ${CMAKE_CURRENT_SOURCE_DIR})
set(CBD ${CMAKE_CURRENT_BINARY_DIR})
add_custom_command(
  OUTPUT ${CBD}/Obj_p.h ${CBD}/Obj.hpp ${CBD}/Obj.cpp ${CBD}/main.cpp
  COMMAND ${CMAKE_COMMAND} -E copy ${CSD}/../Obj_p.h ${CBD}/Obj_p.h
  COMMAND ${CMAKE_COMMAND} -E copy ${CSD}/../Obj.hpp ${CBD}/Obj.hpp
  COMMAND ${CMAKE_COMMAND} -E copy ${CSD}/../Obj.cpp ${CBD}/Obj.cpp
  COMMAND ${CMAKE_COMMAND} -E copy ${CSD}/../main.cpp ${CBD}/main.cpp)

# Generate moc files externally
qtx_wrap_cpp(mocCMP0071OldMoc ${CBD}/Obj.hpp ${CBD}/Obj_p.h)
add_executable(mocCMP0071Old ${CBD}/Obj.cpp ${CBD}/main.cpp ${mocCMP0071OldMoc})
target_link_libraries(mocCMP0071Old ${QT_LIBRARIES})
set_target_properties(mocCMP0071Old PROPERTIES AUTOMOC ON)
