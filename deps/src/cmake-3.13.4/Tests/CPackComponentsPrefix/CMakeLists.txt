cmake_minimum_required(VERSION 3.2)
project(CPackComponentsPrefix NONE)

install(FILES file-runtime.txt
        DESTINATION bin COMPONENT Runtime)
install(FILES file-development.txt
        DESTINATION lib COMPONENT Development)

set(CPACK_PACKAGE_CONTACT "None") # mandatory for DEB generator
set(CPACK_COMPONENT_INCLUDE_TOPLEVEL_DIRECTORY 1)
set(CPACK_COMPONENTS_ALL Development)
set(CPACK_ARCHIVE_COMPONENT_INSTALL 1)
set(CPACK_PACKAGING_INSTALL_PREFIX "/opt/My-1.0")
include(CPack)
