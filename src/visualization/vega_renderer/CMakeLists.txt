project( vega_renderer )

if(APPLE AND NOT TC_BUILD_IOS)

   add_definitions(
      -DOBJROOT=${CMAKE_CURRENT_BINARY_DIR}
   )

   set( VEGA_RENDERER_PUBLIC_HEADERS
      VegaRenderer.h
   )

   set( VEGA_RENDERER_SOURCES
      JSCanvas.m
      JSDocument.m
      VegaRenderer.m
      colors.m
   )

   set( VEGA_RENDERER_STATIC_DEPENDENCIES
   )

   set( VEGA_RENDERER_DEPENDENCIES
      ${FOUNDATION}
      ${APPKIT}
      ${CORE_GRAPHICS}
      ${JAVASCRIPT_CORE}
   )

   ##
   ## Preprocessed JavaScript artifacts
   ##
   set(JS_FILE_EXT "js")
   if (CMAKE_BUILD_TYPE STREQUAL "Release")
      set(JS_FILE_EXT "min.js")
   endif()
   add_custom_command(
      OUTPUT
         ${CMAKE_CURRENT_BINARY_DIR}/vega-5.4.0.${JS_FILE_EXT}.h
         ${CMAKE_CURRENT_BINARY_DIR}/vega-lite-3.3.0.${JS_FILE_EXT}.h
      COMMENT "Running xxd to generate headers from JavaScript artifacts"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMAND xxd -i vega-5.4.0.${JS_FILE_EXT} ${CMAKE_CURRENT_BINARY_DIR}/vega-5.4.0.${JS_FILE_EXT}.h
      COMMAND xxd -i vega-lite-3.3.0.${JS_FILE_EXT} ${CMAKE_CURRENT_BINARY_DIR}/vega-lite-3.3.0.${JS_FILE_EXT}.h
      DEPENDS
         ${CMAKE_CURRENT_SOURCE_DIR}/vega-5.4.0.${JS_FILE_EXT}
         ${CMAKE_CURRENT_SOURCE_DIR}/vega-lite-3.3.0.${JS_FILE_EXT}
      VERBATIM
   )
   add_custom_target(
      preprocessed_javascript
      DEPENDS
         ${CMAKE_CURRENT_BINARY_DIR}/vega-5.4.0.${JS_FILE_EXT}.h
         ${CMAKE_CURRENT_BINARY_DIR}/vega-lite-3.3.0.${JS_FILE_EXT}.h
   )

   ##
   ## VegaRenderer
   ##

   make_library( vega_renderer OBJECT
      SOURCES
         ${VEGA_RENDERER_SOURCES}
      REQUIRES
         ${VEGA_RENDERER_DEPENDENCIES}
   )

   add_dependencies(
      vega_renderer
      preprocessed_javascript
   )

   target_compile_options(vega_renderer PUBLIC "-fobjc-arc")

endif()
