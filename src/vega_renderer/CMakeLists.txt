project( vega_renderer )

if(APPLE)

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
      -DOBJROOT=${CMAKE_CURRENT_BINARY_DIR}/VegaRenderer
   )

   set( VEGA_RENDERER_INCLUDE_DIRS
   VegaRenderer
   )

   set( VEGA_RENDERER_PUBLIC_HEADERS
   VegaRenderer/VegaRenderer.h
   )

   set( VEGA_RENDERER_PROJECT_HEADERS
   )

   set( VEGA_RENDERER_PRIVATE_HEADERS
   )

   set( VEGA_RENDERER_SOURCES
   VegaRenderer/JSCanvas.m
   VegaRenderer/VegaRenderer.m
   VegaRenderer/colors.m
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
   file(MAKE_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/VegaRenderer)
   add_custom_command(
      OUTPUT
         ${CMAKE_CURRENT_BINARY_DIR}/VegaRenderer/vega-4.4.0.min.js.h
         ${CMAKE_CURRENT_BINARY_DIR}/VegaRenderer/vega-lite-3.0.0-rc10.min.js.h
      COMMENT "Running xxd to generate headers from JavaScript artifacts"
      WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/VegaRenderer
      COMMAND xxd -i vega-4.4.0.min.js ${CMAKE_CURRENT_BINARY_DIR}/VegaRenderer/vega-4.4.0.min.js.h
      COMMAND xxd -i vega-lite-3.0.0-rc10.min.js ${CMAKE_CURRENT_BINARY_DIR}/VegaRenderer/vega-lite-3.0.0-rc10.min.js.h
      VERBATIM
   )
   add_custom_target(
      preprocessed_javascript
      DEPENDS
         ${CMAKE_CURRENT_BINARY_DIR}/VegaRenderer/vega-4.4.0.min.js.h
         ${CMAKE_CURRENT_BINARY_DIR}/VegaRenderer/vega-lite-3.0.0-rc10.min.js.h
   )

   ##
   ## VegaRenderer
   ##

   add_library( VegaRenderer OBJECT
   ${VEGA_RENDERER_SOURCES}
   ${VEGA_RENDERER_PUBLIC_HEADERS}
   ${VEGA_RENDERER_PROJECT_HEADERS}
   ${VEGA_RENDERER_PRIVATE_HEADERS}
   ${VEGA_RENDERER_RESOURCES}
   )

   add_dependencies(
      VegaRenderer
      preprocessed_javascript
   )

   target_link_libraries(
      VegaRenderer
      ${FOUNDATION_LIBRARY}
      ${APPKIT_LIBRARY}
      ${COREGRAPHICS_LIBRARY}
      ${JAVASCRIPTCORE_LIBRARY}
      ${CORETEXT_LIBRARY}
   )

   target_include_directories(
      VegaRenderer
      PUBLIC
      VegaRenderer
   )

   target_compile_options(VegaRenderer PUBLIC "-fobjc-arc")

endif()