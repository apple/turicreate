
target_sources(privatelib PRIVATE subdir_empty_1.cpp
                                  "${CMAKE_CURRENT_LIST_DIR}/subdir_empty_2.cpp"
                                  ../empty_1.cpp
                                  "${CMAKE_CURRENT_LIST_DIR}/../empty_2.cpp")
