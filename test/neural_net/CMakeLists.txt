project(neural_net_test)


if(APPLE AND 
   HAS_MPS AND 
   NOT TC_BUILD_IOS AND 
   NOT TC_BASE_SDK_VERSION VERSION_LESS 10.15)

  make_library(style_transfer_test_utils
    SOURCES
      style_transfer_testing.mm
      style_transfer_testing_utils.mm
    REQUIRES
      unity_shared_for_testing
  )

  target_compile_options(style_transfer_test_utils 
    PUBLIC 
      "-fobjc-arc -Wno-nullability-completeness -Wno-shadow-ivar"
  )
  
  make_boost_test(style_transfer.cxx
    REQUIRES
      style_transfer_test_utils
  )
  
endif()