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
   add_custom_command(
      OUTPUT
         ${CMAKE_CURRENT_BINARY_DIR}/vega-4.4.0.min.js.h
         ${CMAKE_CURRENT_BINARY_DIR}/vega-lite-3.0.0-rc10.min.js.h
      COMMENT "Running xxd to generate headers from JavaScript artifacts"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
      COMMAND xxd -i vega-4.4.0.min.js ${CMAKE_CURRENT_BINARY_DIR}/vega-4.4.0.min.js.h
      COMMAND xxd -i vega-lite-3.0.0-rc10.min.js ${CMAKE_CURRENT_BINARY_DIR}/vega-lite-3.0.0-rc10.min.js.h
      VERBATIM
   )
   add_custom_target(
      preprocessed_javascript
      DEPENDS
         ${CMAKE_CURRENT_BINARY_DIR}/vega-4.4.0.min.js.h
         ${CMAKE_CURRENT_BINARY_DIR}/vega-lite-3.0.0-rc10.min.js.h
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
