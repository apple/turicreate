PROJECT(CMCompress)

ADD_LIBRARY(cmcompress cmcompress.c)

INSTALL(FILES Copyright.txt DESTINATION ${CMAKE_DOC_DIR}/cmcompress)
