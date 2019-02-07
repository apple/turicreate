cmake_minimum_required(VERSION 2.8.12)
project(aws-cpp-sdk-s3)

  file(GLOB AWS_S3_HEADERS
    "include/aws/s3/*.h"
  )

  file(GLOB AWS_S3_MODEL_HEADERS
    "include/aws/s3/model/*.h"
  )

  file(GLOB AWS_S3_SOURCE
    "source/*.cpp"
  )

  file(GLOB AWS_S3_MODEL_SOURCE
    "source/model/*.cpp"
  )

  file(GLOB S3_UNIFIED_HEADERS
    ${AWS_S3_HEADERS}
    ${AWS_S3_MODEL_HEADERS}
  )

  file(GLOB S3_UNITY_SRC
    ${AWS_S3_SOURCE}
    ${AWS_S3_MODEL_SOURCE}
  )

  if(ENABLE_UNITY_BUILD)
    enable_unity_build("S3" S3_UNITY_SRC)
  endif()

  file(GLOB S3_SRC
    ${S3_UNIFIED_HEADERS}
    ${S3_UNITY_SRC}
  )

  if(WIN32)
    #if we are compiling for visual studio, create a sane directory tree.
    if(MSVC)
      source_group("Header Files\\aws\\s3" FILES ${AWS_S3_HEADERS})
      source_group("Header Files\\aws\\s3\\model" FILES ${AWS_S3_MODEL_HEADERS})
      source_group("Source Files" FILES ${AWS_S3_SOURCE})
      source_group("Source Files\\model" FILES ${AWS_S3_MODEL_SOURCE})
    endif(MSVC)
  endif()

  set(S3_INCLUDES
    "${CMAKE_CURRENT_SOURCE_DIR}/include/"
    "${CORE_DIR}/include/"
  )

  include_directories(${S3_INCLUDES})

  if(USE_WINDOWS_DLL_SEMANTICS AND BUILD_SHARED_LIBS)
    add_definitions("-DAWS_S3_EXPORTS")
  endif()

  add_library(aws-cpp-sdk-s3 ${LIBTYPE} ${S3_SRC})

  target_link_libraries(aws-cpp-sdk-s3 aws-cpp-sdk-core)

  if(SIMPLE_INSTALL)
      install (TARGETS aws-cpp-sdk-s3
               ARCHIVE DESTINATION ${ARCHIVE_DIRECTORY}
               LIBRARY DESTINATION lib
               RUNTIME DESTINATION bin)
  else()
    if(PLATFORM_CUSTOM)
      install_custom_library(aws-cpp-sdk-s3)
    else()
      install (TARGETS aws-cpp-sdk-s3
               ARCHIVE DESTINATION ${ARCHIVE_DIRECTORY}/${SDK_INSTALL_BINARY_PREFIX}/${PLATFORM_INSTALL_QUALIFIER}/\${CMAKE_INSTALL_CONFIG_NAME}
               LIBRARY DESTINATION lib/${SDK_INSTALL_BINARY_PREFIX}/${PLATFORM_INSTALL_QUALIFIER}/\${CMAKE_INSTALL_CONFIG_NAME}
               RUNTIME DESTINATION bin/${SDK_INSTALL_BINARY_PREFIX}/${PLATFORM_INSTALL_QUALIFIER}/\${CMAKE_INSTALL_CONFIG_NAME})
    endif()
  endif()

  install (FILES ${AWS_S3_HEADERS} DESTINATION include/aws/s3)
  install (FILES ${AWS_S3_MODEL_HEADERS} DESTINATION include/aws/s3/model)

  if(MSVC)
    install (FILES nuget/aws-cpp-sdk-s3.autopkg DESTINATION nuget)
  endif()



