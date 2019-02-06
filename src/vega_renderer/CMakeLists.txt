project( vega_renderer )

if(APPLE AND NOT TC_BUILD_IOS)

   find_library( FOUNDATION_LIBRARY Foundation)
   message( STATUS "Foundation found at ${FOUNDATION_LIBRARY}")

   find_library( APPKIT_LIBRARY AppKit)
   message( STATUS "AppKit found at ${APPKIT_LIBRARY}")

   find_library( COREGRAPHICS_LIBRARY CoreGraphics)
   message( STATUS "CoreGraphics found at ${COREGRAPHICS_LIBRARY}")

   find_library( JAVASCRIPTCORE_LIBRARY JavaScriptCore)
   message( STATUS "JavaScriptCore found at ${JAVASCRIPTCORE_LIBRARY}")

   find_library( CORETEXT_LIBRARY CoreText)
   message( STATUS "CoreText found at ${CORETEXT_LIBRARY}")

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
      ${FOUNDATION_LIBRARY}
      ${APPKIT_LIBRARY}
      ${COREGRAPHICS_LIBRARY}
      ${JAVASCRIPTCORE_LIBRARY}
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

   make_library( vega_renderer
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
