include(ExternalProject)

file(WRITE ${CMAKE_CURRENT_BINARY_DIR}/zzzz_tmp.txt "Dummy file")
file(MD5   ${CMAKE_CURRENT_BINARY_DIR}/zzzz_tmp.txt md5hash)
ExternalProject_Add(Subst
    URL           file://${CMAKE_CURRENT_BINARY_DIR}/zzzz_tmp.txt
    URL_HASH      MD5=${md5hash}
    DOWNLOAD_NO_EXTRACT ON
    DOWNLOAD_DIR  ${CMAKE_CURRENT_BINARY_DIR}/xxxx_dwn
    SOURCE_DIR    ${CMAKE_CURRENT_BINARY_DIR}/xxxx_src
    SOURCE_SUBDIR yyyy_subdir
    BINARY_DIR    ${CMAKE_CURRENT_BINARY_DIR}/xxxx_bin
    INSTALL_DIR   ${CMAKE_CURRENT_BINARY_DIR}/xxxx_install
    TMP_DIR       ${CMAKE_CURRENT_BINARY_DIR}/xxxx_tmp
    CONFIGURE_COMMAND ${CMAKE_COMMAND} -E echo "Download dir  = <DOWNLOAD_DIR>"
              COMMAND ${CMAKE_COMMAND} -E echo "Download file = <DOWNLOADED_FILE>"
              COMMAND ${CMAKE_COMMAND} -E echo "Source dir    = <SOURCE_DIR>"
              COMMAND ${CMAKE_COMMAND} -E echo "Source subdir = <SOURCE_SUBDIR>"
              COMMAND ${CMAKE_COMMAND} -E echo "Binary dir    = <BINARY_DIR>"
              COMMAND ${CMAKE_COMMAND} -E echo "Install dir   = <INSTALL_DIR>"
              COMMAND ${CMAKE_COMMAND} -E echo "Tmp dir       = <TMP_DIR>"
    BUILD_COMMAND ""
    INSTALL_COMMAND ""
    TEST_COMMAND ""
)
