project(unity_test_toolkits)

make_boost_test(test_model_spec.cxx
  REQUIRES
    unity_shared_for_testing
)

make_boost_test(test_image_augmentation.cxx REQUIRES unity_shared_for_testing)

if(APPLE AND HAS_MPS AND NOT TC_BUILD_IOS)
  make_boost_test(test_mps_image_augmentation.cxx REQUIRES unity_shared_for_testing)
endif()
