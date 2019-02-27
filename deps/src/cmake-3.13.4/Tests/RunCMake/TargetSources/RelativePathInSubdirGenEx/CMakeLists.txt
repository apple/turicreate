
target_sources(genexlib PUBLIC $<1:${CMAKE_CURRENT_LIST_DIR}/subdir_empty_1.cpp>
                               $<1:${CMAKE_CURRENT_LIST_DIR}/../empty_1.cpp>
                        PRIVATE $<1:empty_2.cpp>)
