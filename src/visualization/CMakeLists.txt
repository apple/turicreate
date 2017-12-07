project(Turi)

if(APPLE)
  if(${TC_BUILD_VISUALIZATION_CLIENT})

    add_custom_target(visualization_client
      COMMAND xcodebuild -project "${CMAKE_CURRENT_SOURCE_DIR}/Turi Create Visualization.xcodeproj/" -configuration ${CMAKE_BUILD_TYPE} SYMROOT=${CMAKE_CURRENT_BINARY_DIR}
      COMMENT "Building visualization client via xcodebuild"
  )
  else()
    make_empty_library(visualization_client)
  endif()
else()
  make_empty_library(visualization_client)
endif()
