project(vega_renderer_test)

if(APPLE)

    # sets XCTest_LIBRARIES
    include(FindXCTest)

    find_library( WEBKIT_LIBRARY WebKit )
    message( STATUS "WebKit found at ${WEBKIT_LIBRARY}")

    add_definitions(-DPROJECT_DIR=${CMAKE_CURRENT_SOURCE_DIR})
    make_library(
        vega_renderer_test_utils
        SOURCES
            base_fixture.mm
            vega_webkit_renderer.mm
        REQUIRES
            vega_renderer
            ${WEBKIT_LIBRARY}
            ${XCTest_LIBRARIES}
    )

    make_boost_test(vega_example_tests.cxx REQUIRES vega_renderer_test_utils)
    make_boost_test(vega_lite_example_tests.cxx REQUIRES vega_renderer_test_utils)
    make_boost_test(turicreate_example_tests.cxx REQUIRES vega_renderer_test_utils)

endif()
