project(Turi)

make_library(style_transfer_toolkit_test_utils
  SOURCES
    utils.cpp
  REQUIRES
  	unity_style_transfer
    unity_shared_for_testing
)

make_boost_test(test_style_transfer_data_iterator.cxx
  REQUIRES
    style_transfer_toolkit_test_utils
)

make_boost_test(test_style_transfer_model_definition.cxx
  REQUIRES
    style_transfer_toolkit_test_utils
)
