project(TuriCreateObjC)

if(APPLE AND HAS_MLCUSTOMMODEL)

  make_library(Recommender
    SOURCES
      TuriCreateObjC.m
      recommender_initialization.cpp
    REQUIRES
      ${FOUNDATION}
      ${CORE_ML}
      TuriCreate_static
    SHARED
    SHARED_ALL_DEFINED
    EXPORT_OSX_MAP_FILE "${CMAKE_CURRENT_SOURCE_DIR}/symbol_exports.ver"
    DEAD_STRIP
  )

  add_library(objcapi OBJECT
    TuriCreateObjC.m
  )

  target_compile_options(objcapi PUBLIC "-fobjc-arc")
  target_compile_options(Recommender PUBLIC "-fobjc-arc")

  if (CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_command(TARGET Recommender
      POST_BUILD
      COMMAND strip -Sx "$<TARGET_FILE:Recommender>"
      COMMENT "Stripping Objective-C Recommender library"
    )
  endif()

endif()
