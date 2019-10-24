project(neural_net_test)

make_library(style_transfer_toolkit_test_utils
  SOURCES
    utils.mm 
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

if(APPLE AND 
   NOT TC_BUILD_IOS AND 
   NOT TC_BASE_SDK_VERSION VERSION_LESS 10.15)
  make_boost_test(test_style_transfer.cxx
    REQUIRES
      style_transfer_toolkit_test_utils
  )
endif()