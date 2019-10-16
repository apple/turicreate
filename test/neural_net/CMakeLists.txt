project(neural_net_test)


if(APPLE AND 
   HAS_MPS AND 
   NOT TC_BUILD_IOS AND 
   NOT TC_BASE_SDK_VERSION VERSION_LESS 10.15)

  make_boost_test(style_transfer.cxx)
  
endif()