cmake_minimum_required(VERSION 3.3)
project(VSNsightTegra C CXX)

set(CMAKE_ANDROID_ARCH armv7-a)
set(CMAKE_ANDROID_STL_TYPE stlport_shared)
set(CMAKE_ANDROID_API_MIN 9)
set(CMAKE_ANDROID_API 15)
set(CMAKE_ANDROID_GUI 1)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/lib")

set(FIRST_C_FILES
  jni/first.c
  jni/first.h
  )

source_group(jni FILES ${FIRST_C_FILES})
add_library(twolib-first ${FIRST_C_FILES})

set(SECOND_C_FILES
  jni/second.c
  )
set(SECOND_JAVA_FILES
  src/com/example/twolibs/TwoLibs.java
  )
set(SECOND_RES_FILES
  res/values/strings.xml
  )
set(SECOND_ANDROID_FILES
  AndroidManifest.xml
  )

source_group(jni FILES ${SECOND_C_FILES})
source_group(res\\values FILES ${SECOND_RES_FILES})
source_group(src\\com\\example\\twolibs FILES ${SECOND_JAVA_FILES})
add_executable(twolib-second
  ${SECOND_C_FILES}
  ${SECOND_JAVA_FILES}
  ${SECOND_RES_FILES}
  ${SECOND_ANDROID_FILES}
  )
target_include_directories(twolib-second PUBLIC jni)
target_link_libraries(twolib-second twolib-first)
target_link_libraries(twolib-second m) # test linking to library by name

set_property(TARGET twolib-second PROPERTY C_STANDARD 11)
set_target_properties(twolib-second PROPERTIES ANDROID_SKIP_ANT_STEP 1)
set_target_properties(twolib-second PROPERTIES ANDROID_PROGUARD 1)
set_target_properties(twolib-second PROPERTIES ANDROID_PROGUARD_CONFIG_PATH proguard-android.txt)
set_target_properties(twolib-second PROPERTIES ANDROID_SECURE_PROPS_PATH /definitely/insecure)

set_property(TARGET twolib-second PROPERTY ANDROID_NATIVE_LIB_DIRECTORIES $<TARGET_FILE_DIR:twolib-second>)
set_property(TARGET twolib-second PROPERTY ANDROID_NATIVE_LIB_DEPENDENCIES $<TARGET_FILE_NAME:twolib-second>)

set_property(TARGET twolib-second PROPERTY ANDROID_JAR_DIRECTORIES $<TARGET_FILE_DIR:twolib-first>)
