project(vega_renderer_test)

if(APPLE AND NOT TC_BUILD_IOS)

    find_library( WEBKIT_LIBRARY WebKit )
    message( STATUS "WebKit found at ${WEBKIT_LIBRARY}")

    find_library( XCTEST_LIBRARY XCTest )
    message( STATUS "XCTest found at ${XCTEST_LIBRARY}")

    include_directories(${XCTEST_LIBRARY})

    add_definitions(-DPROJECT_DIR=${CMAKE_CURRENT_SOURCE_DIR})

    add_library(vega_renderer_test_utils OBJECT
        base_fixture.mm
        vega_webkit_renderer.mm
    )
    add_dependencies(vega_renderer_test_utils boost)
    target_compile_options(vega_renderer_test_utils PUBLIC "-fobjc-arc")

    set(source_deps
        $<TARGET_OBJECTS:vega_renderer>
        $<TARGET_OBJECTS:vega_renderer_test_utils>
    )

    set(target_deps
        ${WEBKIT_LIBRARY}
        ${XCTEST_LIBRARY}
        ${FOUNDATION}
        ${APPKIT}
        ${CORE_GRAPHICS}
        ${JAVASCRIPTCORE}
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
