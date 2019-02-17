project(vega_renderer_test)

if(APPLE AND NOT TC_BUILD_IOS)

    # sets XCTest_LIBRARIES
    include(FindXCTest)

    find_library( WEBKIT WebKit )
    message( STATUS "WebKit found at ${WEBKIT}")

    add_definitions(-DPROJECT_DIR=${CMAKE_CURRENT_SOURCE_DIR})

    make_library(vega_renderer_test_utils OBJECT
        SOURCES
            base_fixture.mm
            vega_webkit_renderer.mm
        REQUIRES
            vega_renderer
            ${WEBKIT}
            ${XCTest_LIBRARIES}
    )
    target_compile_options(vega_renderer_test_utils PUBLIC "-fobjc-arc")

    set(source_deps
        $<TARGET_OBJECTS:vega_renderer>
        $<TARGET_OBJECTS:vega_renderer_test_utils>
    )

    set(target_deps
        ${WEBKIT}
        ${XCTest_LIBRARIES}
        ${FOUNDATION}
        ${APPKIT}
        ${CORE_GRAPHICS}
        ${JAVASCRIPT_CORE}
    )

    make_boost_test(vega_example_tests.cxx ${source_deps} REQUIRES ${target_deps})
    make_boost_test(vega_lite_example_tests.cxx ${source_deps} REQUIRES ${target_deps})
    make_boost_test(turicreate_example_tests.cxx ${source_deps} REQUIRES ${target_deps})

    add_custom_command(
        TARGET vega_renderer_test_utils PRE_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/examples
                ${CMAKE_CURRENT_BINARY_DIR}/examples
    )

endif()
