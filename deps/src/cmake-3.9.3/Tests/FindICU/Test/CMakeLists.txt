cmake_minimum_required(VERSION 3.1)
project(TestFindICU LANGUAGES CXX)
include(CTest)

find_package(ICU REQUIRED COMPONENTS i18n uc)

add_executable(test_icu_tgt main.cpp)
target_link_libraries(test_icu_tgt ICU::i18n ICU::uc)
add_test(NAME test_icu_tgt COMMAND test_icu_tgt)

add_executable(test_icu_var main.cpp)
target_include_directories(test_icu_var PRIVATE ${ICU_INCLUDE_DIRS})
target_link_libraries(test_icu_var PRIVATE ${ICU_LIBRARIES})
add_test(NAME test_icu_var COMMAND test_icu_var)
