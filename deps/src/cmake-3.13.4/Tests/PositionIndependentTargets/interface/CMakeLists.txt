
add_library(piciface INTERFACE)
set_property(TARGET piciface PROPERTY INTERFACE_POSITION_INDEPENDENT_CODE ON)

add_executable(test_empty_iface "${CMAKE_CURRENT_SOURCE_DIR}/../pic_main.cpp")
target_link_libraries(test_empty_iface piciface)

add_library(sharedlib SHARED "${CMAKE_CURRENT_SOURCE_DIR}/../pic_lib.cpp")
target_link_libraries(sharedlib piciface)
set_property(TARGET sharedlib PROPERTY DEFINE_SYMBOL PIC_TEST_BUILD_DLL)

add_executable(test_iface_via_shared "${CMAKE_CURRENT_SOURCE_DIR}/../pic_main.cpp")
target_link_libraries(test_iface_via_shared sharedlib)

add_library(sharedlibpic SHARED "${CMAKE_CURRENT_SOURCE_DIR}/../pic_lib.cpp")
set_property(TARGET sharedlibpic PROPERTY INTERFACE_POSITION_INDEPENDENT_CODE ON)
set_property(TARGET sharedlibpic PROPERTY DEFINE_SYMBOL PIC_TEST_BUILD_DLL)

add_library(shared_iface INTERFACE)
target_link_libraries(shared_iface INTERFACE sharedlibpic)

add_executable(test_shared_via_iface "${CMAKE_CURRENT_SOURCE_DIR}/../pic_main.cpp")
target_link_libraries(test_shared_via_iface shared_iface)

add_executable(test_shared_via_iface_non_conflict "${CMAKE_CURRENT_SOURCE_DIR}/../pic_main.cpp")
set_property(TARGET test_shared_via_iface_non_conflict PROPERTY POSITION_INDEPENDENT_CODE ON)
target_link_libraries(test_shared_via_iface_non_conflict shared_iface)
